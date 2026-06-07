# Surface Hole Stencil

## 概要

Surface Hole Stencil は、メッシュを実際に破壊せずに、床や壁などの表面に穴が開いたように見せるための描画機能です。

この機能は stencil buffer を使います。

1. `HoleMask` オブジェクトが、自分のメッシュが重なったピクセルに stencil 値を書き込む。
2. `HoleReceiver` オブジェクト、たとえば床が、stencil 値のあるピクセルだけ描画をスキップする。
3. 結果として、床の一部が抜けて穴が開いたように見える。

これは見た目だけの穴です。メッシュ形状、コリジョン、ナビメッシュ、レイキャスト形状、シャドウキャスター形状は変わりません。

## 使い方

`BaseGameObject` の `Draw Config > Surface Stencil Role` から設定します。

設定値は以下です。

- `None`: 通常描画。
- `Hole Mask`: stencil だけを書き込む。色も深度も描画しない。
- `Hole Receiver`: stencil が穴の値ではない場所だけ通常描画する。

典型的な使い方は以下です。

```text
ひび割れ・穴形状のマスク用プレーン -> Hole Mask
壊れる床                           -> Hole Receiver
その他のオブジェクト               -> None
```

穴の形状は `Hole Mask` オブジェクトに貼ったテクスチャの alpha で決まります。`HoleMask.PS.hlsl` で透明ピクセルを `discard` するため、透明部分は stencil を書き込みません。

## 実装内容

### Depth/Stencil Target

メインの offscreen render target の depth resource を `DXGI_FORMAT_R24G8_TYPELESS` に変更し、DSV/PSO 側では `DXGI_FORMAT_D24_UNORM_S8_UINT` を使うようにしました。

関連ファイル:

- `Engine/Graphics/RenderTarget/OffscreenRT/OffscreenRenderTarget.cpp`
- `Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h`

毎フレーム、depth と stencil を同時に clear します。

```cpp
D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL
```

stencil を clear しないと、前フレームや別 render target pass の穴マスクが次の描画に残ってしまうためです。

### Render Role

`DrawConfig` に `SurfaceStencilRole` を追加しました。

関連ファイル:

- `Engine/Objects/3D/Actor/SceneObject.h`

`SurfaceStencilRole` は `BaseGameObjectConfig` で保存・読み込みされ、`BaseGameObject::ShowGui()` から編集できます。

関連ファイル:

- `Data/Engine/Configs/Scene/Objects/BaseGameObject/BaseGameObjectConfig.h`
- `Engine/Objects/3D/Actor/BaseGameObject.cpp`

### Pipeline State Object

以下の object pipeline variant を追加しました。

- `HoleMaskObject3D`
- `HoleReceiverObject3D`

関連ファイル:

- `Engine/Graphics/Pipeline/Pso/PsoDetails.h`
- `Engine/Graphics/Pipeline/Presets/PipelinePresets.cpp`
- `Engine/Graphics/Pipeline/Presets/PipelinePresets.h`
- `Engine/Graphics/Pipeline/Service/PipelineService.cpp`
- `Engine/Graphics/Pipeline/Service/PipelineService.h`

`HoleMaskObject3D` の役割:

- `Object3d.VS.hlsl` を使う。
- `HoleMask.PS.hlsl` を使う。
- stencil を有効にする。
- `Comp Always` 相当で常に stencil test を通す。
- `Pass Replace` 相当で stencil に値を書き込む。
- stencil 値 `1` を書き込む。
- color write は無効。
- depth write は無効。

`HoleReceiverObject3D` の役割:

- 通常の `Object3d.PS.hlsl`、または runtime material graph の pixel shader を使う。
- stencil を有効にする。
- `Comp NotEqual` 相当で stencil 値 `1` の場所を描画しない。
- 通常の Object3D と同じライティング・マテリアル経路を維持する。

### Shader

関連ファイル:

- `Resources/shaders/Core/HoleMask.PS.hlsl`

`HoleMask.PS.hlsl` は、マスクテクスチャの alpha を見て透明部分を捨てます。

```hlsl
float alpha = gTexture.Sample(gSampler, input.texcoord).a;
if(alpha <= 0.5f) {
    discard;
}
```

PSO 側で color write を無効にしているため、この pixel shader は「どのピクセルに stencil を書くか」を決めるためだけに存在します。

### ModelRenderer への接続

関連ファイル:

- `Engine/Renderer/Model/ModelRenderer.cpp`

`BuildStaticBatches()` で `SurfaceStencilRole` を見て、static model instance を描画 bucket に振り分けます。

```text
None         -> Object3d
HoleMask     -> HoleMaskObject3D
HoleReceiver -> HoleReceiverObject3D
```

`PipelineTag::Object` の enum 順で `HoleMaskObject3D` を通常の `Object3D` より前に置いています。`ModelRenderer` は `std::map<PipelineKey, ...>` で batch を保持しているため、この順序により `HoleMask` が `HoleReceiver` より先に描画されます。

これは重要です。`HoleReceiver` は、先に書かれた stencil 値を参照して描画をスキップするためです。

描画時には、`HoleMaskObject3D` と `HoleReceiverObject3D` のときだけ `OMSetStencilRef(1)` を設定します。通常描画では `0` を設定します。

runtime material graph を使う `HoleReceiver` には専用の generated-material PSO cache path を追加しています。DirectX 12 では stencil state が PSO に含まれるため、pixel shader だけを差し替えても stencil state は変えられないからです。

## プログラム的な工夫点

### PSO variant として実装している理由

DirectX 12 では depth/stencil state は graphics pipeline state object に含まれます。

つまり、draw call の直前に「このオブジェクトだけ stencil test を変える」というような切り替えは、定数バッファのようにはできません。

そのため、以下のように PSO を分けています。

```text
通常描画用       -> Object3D
stencil 書き込み -> HoleMaskObject3D
stencil test 用  -> HoleReceiverObject3D
```

### GraphicsPipelineDesc の hash に stencil state を含めた理由

`GraphicsPipelineDesc` は PSO cache の key として使われています。

今回、同じ vertex shader / pixel shader に見えても、stencil state が違う PSO が必要になりました。

そのため、`GraphicsPipelineDesc::operator==` と `GraphicsPipelineDesc::Hash()` に stencil 関連の値を含めています。

これを入れないと、`Object3D`、`HoleMask`、`HoleReceiver` が誤って同じ PSO を再利用してしまう可能性があります。

### static model だけに対応している理由

現時点では static model の batch だけを `SurfaceStencilRole` で振り分けています。

skinned model にも同じ仕組みは追加できますが、最初の実装では床や壁などの静的な破壊表現を優先し、実装範囲を小さくしています。

### 通常オブジェクトへの負荷

通常の static object は、batch 構築時に `SurfaceStencilRole` の enum check を 1 回受けるだけです。

`HoleMask` または `HoleReceiver` に設定されていない限り、通常の `Object3D` PSO で描画されます。

実際に追加コストが発生するのは以下です。

- `HoleMask` オブジェクトの追加 draw。
- `HoleReceiver` オブジェクトの stencil 有効 PSO。
- runtime material graph receiver 用の追加 PSO cache。

## 他の実装方法との比較

### 本物のメッシュ破壊

メッシュ破壊は、実際に mesh を切断・再生成する方法です。

利点:

- 見た目、コリジョン、シャドウを破壊形状に合わせやすい。
- 物理的に正しい表現に近い。

欠点:

- runtime mesh slicing や mesh generation が必要。
- collider や navmesh の更新が必要。
- CPU/GPU の実装負荷が大きい。
- 保存・復元・デバッグが難しい。

現在のエンジンには mesh destruction 機能がないため、今回の stencil cutout は見た目を先に実現するための現実的な方法です。

### 床 shader 側で alpha discard する方法

床 material に穴位置や半径を渡し、床 shader 内で `discard` する方法です。

利点:

- stencil buffer を使わなくてもよい。
- 円形などの解析的な穴には向いている。

欠点:

- 穴を受けるすべての shader に穴処理を入れる必要がある。
- 複数穴や任意形状の破壊跡 texture 管理が複雑。
- material graph 生成 shader にも穴処理を注入する必要がある。
- 床以外の receiver に拡張しづらい。

今回の stencil 方式では、receiver shader の中身をほぼ変えずに済みます。

### Render Texture に mask を描く方法

別の render texture に穴マスクを描き、床 shader がその texture を sample する方法です。

利点:

- stencil より多くの情報を保存できる。
- 広い terrain や永続的な汚れ・破壊跡に向いている。

欠点:

- world space / UV space の投影ルールが必要。
- receiver shader が mask texture を sample する必要がある。
- 追加 texture memory と同期処理が必要。

今回のような screen-space の見た目の穴には、stencil の方が実装が軽いです。

### Decal / Render Feature 方式

Unity や Unreal Engine では、この種の表現は layer、renderer feature、decal、custom depth/stencil pass などで実装されることが多いです。

今回の実装は、それらの最小構成に近いです。

- `HoleMask` は専用の描画 role。
- `HoleReceiver` も専用の描画 role。
- `ModelRenderer` が role を見て PSO bucket を分ける。

将来的には、`ModelRenderer` の中に穴専用処理を増やし続けるより、`SurfaceHoleRenderer` や `RenderFeature` のような専用システムに分離すると、Unity/Unreal に近い設計になります。

## 制限事項

- 穴は見た目だけで、コリジョンは変わらない。
- シャドウキャスター形状は変わらない。
- 現時点では static model のみ `SurfaceStencilRole` に対応。
- stencil は 8bit の共有 buffer なので、今後別の stencil 機能を追加する場合は値の割り当てルールが必要。
- 半透明 receiver は描画順の追加調整が必要になる可能性がある。
- 穴内部の暗い面、背景投影、内部 texture 描画はまだ未実装。

## 今後の拡張案

アプリ側から直接 `HoleMask` 用プレーンを毎回作るのではなく、以下のような helper API を追加すると扱いやすくなります。

```cpp
CreateSurfaceHole(position, radius, maskTexture);
```

内部では以下を行うのが理想です。

1. mask plane を生成する。
2. `SurfaceStencilRole::HoleMask` を設定する。
3. 対象 surface に沿うように位置・回転を調整する。
4. 必要なら穴内部用の暗い plane や texture object を生成する。

この形にすると、アプリ開発者は stencil の仕組みを直接意識せず、「この位置に穴を作る」という意味で機能を使えます。
