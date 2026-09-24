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

namespace zlpanel::multilingual::zh_Hans {
    inline constexpr std::array kTexts = {
        "切换界面设置面板。",
        // preset related
        "切换预设浏览器。",
        "在系统文件管理器中打开预设文件夹。",
        "删除所选预设分组。",
        "删除所选预设。",
        "关闭预设浏览器。",
        // analyzer related
        "切换分析仪设置面板。",
        "选择分析仪的峰值检测模式。",
        "选择波形滚动模式。",
        "切换输入幅度曲线的显示。",
        "切换输出幅度曲线的显示。",
        "切换限制器增益衰减曲线的显示。",
        "选择分析仪窗口显示的持续时间。",
        "选择分析仪显示的最小分贝下限。",
        "切换电平表栏的显示。",
        "切换响度与数值读数面板的显示。",
        // top controls
        "调节最大输出上限电平。",
        "选择过采样倍率以减少混叠失真与采样间峰值。",
        "切换真峰值限制以防止采样间削波。",
        "切换差值试听。",
        "切换插件旁通状态。",
        // input gain controls
        "选择增益联动模式。\nInput：仅调节输入增益。\nLinked：同步衰减输出以匹配输入增益。",
        "调节限制器的输入增益。",
        // bottom controls
        "调节主包络的前瞻时间以预判瞬态。",
        "调节主包络的自适应恢复时间。",
        "调节持续支撑包络的启动时间。",
        "调节持续支撑包络的释放时间。",
        "调节通道间允许的最大增益衰减差值。",
        // values
        "切换数值读数设置面板。",
        "显示保持的最大真峰值电平。",
        "显示当前（左）和平均（右）立体声相位相关度。",
        "显示当前（左）和最大（右）瞬时响度。",
        "显示当前（左）和最大（右）短期响度。",
        "显示响度范围。括号表示当前读数为暂定值。",
        "显示综合响度。",
        " "
    };
}
