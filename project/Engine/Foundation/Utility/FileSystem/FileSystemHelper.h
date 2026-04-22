#pragma once
#include <filesystem>
#include <string>
#include <optional>
#include <unordered_map>

class FileSystemHelper {
public:
	static bool CreateDirectoryPath(const std::string& path);

	/// <summary>
	/// 指定されたディレクトリ配下から再帰的にファイルを検索
	/// </summary>
	/// <param name="searchDir">検索対象のディレクトリ</param>
	/// <param name="fileName">検索するファイル名</param>
	/// <returns>見つかったファイルの完全パス、見つからない場合は空</returns>
	static std::optional<std::string> FindFileRecursive(const std::string& searchDir, const std::string& fileName);

	/// <summary>
	/// 指定されたディレクトリ配下から再帰的にワイド文字のファイルを検索
	/// </summary>
	static std::optional<std::wstring> FindFileRecursiveW(const std::wstring& searchDir, const std::wstring& fileName);

	/// <summary>
	/// ディレクトリをスキャンしてファイルキャッシュを構築
	/// </summary>
	/// <param name="searchDir">スキャン対象のディレクトリ</param>
	/// <returns>ファイル名をキーとしたフルパスのマップ</returns>
	static std::unordered_map<std::string, std::string> BuildFileCache(const std::string& searchDir);

	/// <summary>
	/// ディレクトリをスキャンしてファイルキャッシュを構築（ワイド文字版）
	/// </summary>
	static std::unordered_map<std::wstring, std::wstring> BuildFileCacheW(const std::wstring& searchDir);
};