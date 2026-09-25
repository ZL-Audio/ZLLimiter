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

namespace zlpanel::multilingual::it {
    inline constexpr std::array kTexts = {
        "Mostra/nasconde il pannello delle impostazioni dell'interfaccia.",
        // preset related
        "Mostra/nasconde il browser dei preset.",
        "Apre la cartella dei preset nel gestore file di sistema.",
        "Elimina il gruppo di preset selezionato.",
        "Elimina il preset selezionato.",
        "Chiude il browser dei preset.",
        // analyzer related
        "Mostra/nasconde il pannello delle impostazioni dell'analizzatore.",
        "Seleziona la modalità di rilevamento del picco per l'analizzatore.",
        "Seleziona la modalità di scorrimento della forma d'onda.",
        "Mostra/nasconde la curva di ampiezza in ingresso.",
        "Mostra/nasconde la curva di ampiezza in uscita.",
        "Mostra/nasconde la curva di riduzione del guadagno del limitatore.",
        "Seleziona la durata visualizzata nella finestra dell'analizzatore.",
        "Seleziona il livello minimo in decibel per la visualizzazione dell'analizzatore.",
        "Mostra/nasconde il pannello dei misuratori di livello.",
        "Mostra/nasconde il pannello del loudness e delle letture numeriche.",
        // top controls
        "Regola il livello massimo di uscita (ceiling).",
        "Seleziona il fattore di sovracampionamento per ridurre l'aliasing e i picchi inter-sample.",
        "Attiva/disattiva la limitazione True Peak per prevenire il clipping inter-sample.",
        "Attiva/disattiva l'ascolto delta.",
        "Attiva/disattiva il bypass del plugin.",
        // input gain controls
        "Seleziona la modalità di collegamento del guadagno.\nInput: regola solo il guadagno in ingresso.\nLinked: attenua anche l'uscita per compensare il guadagno in ingresso.",
        "Regola il guadagno in ingresso che pilota il limitatore.",
        // bottom controls
        "Regola il tempo di lookahead dell'inviluppo principale per anticipare i transienti.",
        "Regola il tempo di recupero adattivo dell'inviluppo principale.",
        "Regola il tempo di attacco dell'inviluppo di supporto al sustain.",
        "Regola il tempo di rilascio dell'inviluppo di supporto al sustain.",
        "Regola la differenza massima consentita nella riduzione del guadagno tra i canali.",
        // values
        "Mostra/nasconde il pannello delle impostazioni di lettura dei valori.",
        "Mostra il livello True Peak massimo mantenuto.",
        "Mostra la correlazione di fase stereo attuale (sinistra) e media (destra).",
        "Mostra il loudness momentaneo attuale (sinistra) e massimo (destra).",
        "Mostra il loudness a breve termine attuale (sinistra) e massimo (destra).",
        "Mostra l'intervallo di loudness. Le parentesi indicano una misurazione provvisoria.",
        "Mostra il loudness integrato.",
        " "
    };
}
