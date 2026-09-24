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

namespace zlpanel::multilingual::zh_Hant {
    inline constexpr std::array kTexts = {
        "切換介面設定面板。",
        // preset related
        "切換預設瀏覽器。",
        "在系統檔案管理器中開啟預設資料夾。",
        "刪除所選預設分組。",
        "刪除所選預設。",
        "關閉預設瀏覽器。",
        // analyzer related
        "切換分析儀設定面板。",
        "選擇分析儀的峰值檢測模式。",
        "選擇波形捲動模式。",
        "切換輸入幅度曲線的顯示。",
        "切換輸出幅度曲線的顯示。",
        "切換限制器增益衰減曲線的顯示。",
        "選擇分析儀視窗顯示的持續時間。",
        "選擇分析儀顯示的最小分貝下限。",
        "切換電平表欄的顯示。",
        "切換響度與數值讀數面板的顯示。",
        // top controls
        "調節最大輸出上限電平。",
        "選擇過取樣倍率以減少混疊失真與取樣間峰值。",
        "切換真峰值限制以防止取樣間削波。",
        "切換差值試聽。",
        "切換插件旁通狀態。",
        // input gain controls
        "選擇增益連動模式。\nInput：僅調節輸入增益。\nLinked：同步衰減輸出以匹配輸入增益。",
        "調節限制器的輸入增益。",
        // bottom controls
        "調節主包絡的前瞻時間以預判瞬態。",
        "調節主包絡的自適應恢復時間。",
        "調節持續支撐包絡的啟動時間。",
        "調節持續支撐包絡的釋放時間。",
        "調節通道間允許的最大增益衰減差值。",
        // values
        "切換數值讀數設定面板。",
        "顯示保持的最大真峰值電平。",
        "顯示當前（左）和平均（右）立體聲相位相關度。",
        "顯示當前（左）和最大（右）瞬時響度。",
        "顯示當前（左）和最大（右）短期響度。",
        "顯示響度範圍。括號表示當前讀數為暫定值。",
        "顯示綜合響度。",
        " "
    };
}
