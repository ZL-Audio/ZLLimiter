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

namespace zlpanel::multilingual::de {
    inline constexpr std::array kTexts = {
        "Blendet das Bedienfeld für UI-Einstellungen ein/aus.",
        // preset related
        "Blendet den Preset-Browser ein/aus.",
        "Öffnet den Preset-Ordner im Dateimanager des Systems.",
        "Löscht die ausgewählte Preset-Gruppe.",
        "Löscht das ausgewählte Preset.",
        "Schließt den Preset-Browser.",
        // analyzer related
        "Blendet das Bedienfeld für Analyzer-Einstellungen ein/aus.",
        "Wählt den Peak-Erkennungsmodus für den Analyzer aus.",
        "Wählt den Scrollmodus für die Wellenform aus.",
        "Blendet die Eingangsamplitudenkurve ein/aus.",
        "Blendet die Ausgangsamplitudenkurve ein/aus.",
        "Blendet die Gain-Reduction-Kurve des Limiters ein/aus.",
        "Wählt die im Analyzer-Fenster angezeigte Zeitdauer aus.",
        "Wählt die minimale Dezibel-Untergrenze für die Analyzer-Anzeige aus.",
        "Blendet das Pegelmesser-Bedienfeld ein/aus.",
        "Blendet das Bedienfeld für Lautheit und numerische Messwerte ein/aus.",
        // top controls
        "Regelt den maximalen Ausgangspegel (Ceiling).",
        "Wählt die Oversampling-Rate aus, um Aliasing und Inter-Sample-Peaks zu reduzieren.",
        "Aktiviert/deaktiviert das True-Peak-Limiting, um Inter-Sample-Clipping zu verhindern.",
        "Aktiviert/deaktiviert das Delta-Abhören.",
        "Aktiviert/deaktiviert den Plugin-Bypass.",
        // input gain controls
        "Wählt den Gain-Verknüpfungsmodus aus.\nInput: Regelt nur die Eingangsverstärkung.\nLinked: Dämpft auch den Ausgang, um die Eingangsverstärkung auszugleichen.",
        "Regelt die Eingangsverstärkung, die den Limiter ansteuert.",
        // bottom controls
        "Regelt die Lookahead-Zeit der Haupthüllkurve, um Transienten vorauszusehen.",
        "Regelt die adaptive Recovery-Zeit der Haupthüllkurve.",
        "Regelt die Attack-Zeit der Sustain-Support-Hüllkurve.",
        "Regelt die Release-Zeit der Sustain-Support-Hüllkurve.",
        "Regelt die maximal zulässige Differenz der Gain-Reduction zwischen den Kanälen.",
        // values
        "Blendet das Bedienfeld für Einstellungen der Messwertanzeige ein/aus.",
        "Zeigt den maximal gehaltenen True-Peak-Pegel an.",
        "Zeigt die aktuelle (links) und durchschnittliche (rechts) Stereo-Phasenkorrelation an.",
        "Zeigt die aktuelle (links) und maximale (rechts) momentane Lautheit an.",
        "Zeigt die aktuelle (links) und maximale (rechts) Kurzzeit-Lautheit an.",
        "Zeigt den Lautheitsbereich an. Klammern kennzeichnen eine vorläufige Messung.",
        "Zeigt die integrierte Lautheit an.",
        " "
    };
}
