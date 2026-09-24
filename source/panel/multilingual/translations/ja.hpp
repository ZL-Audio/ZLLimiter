// Copyright (C) 2026 - zsliu98
// This file is part of ZLLimiter
//
// ZLLimiter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLLimiter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLLimiter. If not, see <https://www.gnu.org/licenses/>.

// This file is also dual licensed under the Apache License, Version 2.0. You may obtain a copy of the License at <http://www.apache.org/licenses/LICENSE-2.0>

#pragma once

#include <array>

namespace zlpanel::multilingual::ja {
    inline constexpr std::array kTexts = {
        "UI設定パネルの表示を切り替えます。",
        // preset related
        "プリセットブラウザの表示を切り替えます。",
        "OSのファイルマネージャーでプリセットフォルダーを開きます。",
        "選択したプリセットグループを削除します。",
        "選択したプリセットを削除します。",
        "プリセットブラウザを閉じます。",
        // analyzer related
        "アナライザー設定パネルの表示を切り替えます。",
        "アナライザーのピーク検出モードを選択します。",
        "波形のスクロールモードを選択します。",
        "入力振幅曲線の表示を切り替えます。",
        "出力振幅曲線の表示を切り替えます。",
        "リミッターのゲインリダクション曲線の表示を切り替えます。",
        "アナライザーウィンドウの表示時間を選択します。",
        "アナライザー表示の下限デシベルを選択します。",
        "レベルメーターパネルの表示を切り替えます。",
        "ラウドネスおよび数値表示パネルの表示を切り替えます。",
        // top controls
        "最大出力レベル（シーリング）を調整します。",
        "オーバーサンプリング倍率を選択し、エイリアシングやサンプル間ピークを低減します。",
        "トゥルーピークリミットを切り替え、サンプル間クリッピングを防止します。",
        "デルタ試聴（差分試聴）を切り替えます。",
        "プラグインのバイパス状態を切り替えます。",
        // input gain controls
        "ゲインリンクモードを選択します。\nInput: 入力ゲインのみを調整します。\nLinked: 入力ゲインに合わせて出力も減衰させます。",
        "リミッターへの入力ゲインを調整します。",
        // bottom controls
        "メインエンベロープのルックアヘッドタイムを調整し、トランジェントを予測します。",
        "メインエンベロープの適応型リカバリータイムを調整します。",
        "サステインサポートエンベロープのアタックタイムを調整します。",
        "サステインサポートエンベロープのリリースタイムを調整します。",
        "チャンネル間で許容されるゲインリダクションの最大差を調整します。",
        // values
        "数値表示設定パネルの表示を切り替えます。",
        "最大トゥルーピークのホールド値を表示します。",
        "現在値（左）と平均値（右）のステレオ位相相関度を表示します。",
        "現在値（左）と最大値（右）のモーメンタリーラウドネスを表示します。",
        "現在値（左）と最大値（右）のショートタームラウドネスを表示します。",
        "ラウドネスレンジを表示します。括弧は暫定的な測定値であることを示します。",
        "インテグレーテッドラウドネスを表示します。",
        " "
    };
}
