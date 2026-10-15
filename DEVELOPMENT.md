# 開発

Visual Studio 2026 / MSVC v145、Windows SDK、C++20を使用します。`BitWorks.slnx` を開き、x64 DebugまたはReleaseでビルドします。出力先はVisual Studioの既定値です。

Dear ImGui v1.92.5の公式ソースとWin32 / Direct3D 11バックエンドを `BitWorks/vendor/imgui` に同梱しています。取得元: https://github.com/ocornut/imgui/tree/v1.92.5 。ビルド時のダウンロードは不要です。

式評価は `BitWorks/Expression.h` に実装しています。32bit以内のリテラルを選択幅へ切り詰め、各演算後も切り詰めます。Signedの除算は64bit中間値を使い、最小値 / -1でも未定義動作を避けます。入力はEnter時だけ反映します。

Developer Command Promptで式のテストを実行できます。

```bat
tests\build.cmd
tests\run-tests.cmd
```

式テストでは優先順位、結合順序、符号、幅による切り詰め、32bit乗算、ゼロ除算、不正構文、入れ子制限を確認します。

アプリは必ず `BitWorks.slnx` 経由でビルドします。`OutDir`・`IntDir`・`TargetName`は上書きしません。x64の場合、最終ファイルはソリューション配下の `x64/Debug` または `x64/Release`、中間ファイルはプロジェクト配下の `BitWorks/x64/Debug` または `BitWorks/x64/Release` になります。実行ファイル名は `BitWorks.exe` です。

テストも専用の `tests/ExpressionTests.slnx` からビルドし、Visual Studio既定の出力先を使用します。
