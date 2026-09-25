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

namespace zlpanel::multilingual::es {
    inline constexpr std::array kTexts = {
        "Muestra/oculta el panel de ajustes de la interfaz.",
        // preset related
        "Muestra/oculta el explorador de preajustes.",
        "Abre la carpeta de preajustes en el administrador de archivos del sistema.",
        "Elimina el grupo de preajustes seleccionado.",
        "Elimina el preajuste seleccionado.",
        "Cierra el explorador de preajustes.",
        // analyzer related
        "Muestra/oculta el panel de ajustes del analizador.",
        "Selecciona el modo de detección de picos para el analizador.",
        "Selecciona el modo de desplazamiento de la forma de onda.",
        "Muestra/oculta la curva de amplitud de entrada.",
        "Muestra/oculta la curva de amplitud de salida.",
        "Muestra/oculta la curva de reducción de ganancia del limitador.",
        "Selecciona la duración mostrada en la ventana del analizador.",
        "Selecciona el nivel mínimo en decibelios para la visualización del analizador.",
        "Muestra/oculta el panel de medidores de nivel.",
        "Muestra/oculta el panel de sonoridad y lecturas numéricas.",
        // top controls
        "Ajusta el nivel máximo de techo de salida (ceiling).",
        "Selecciona el factor de sobremuestreo para reducir el aliasing y los picos entre muestras.",
        "Activa/desactiva la limitación True Peak para evitar el recorte entre muestras.",
        "Activa/desactiva la audición delta.",
        "Activa/desactiva el bypass del plugin.",
        // input gain controls
        "Selecciona el modo de enlace de ganancia.\nInput: ajusta solo la ganancia de entrada.\nLinked: también atenúa la salida para compensar la ganancia de entrada.",
        "Ajusta la ganancia de entrada que alimenta el limitador.",
        // bottom controls
        "Ajusta el tiempo de lookahead de la envolvente principal para anticipar los transitorios.",
        "Ajusta el tiempo de recuperación adaptativo de la envolvente principal.",
        "Ajusta el tiempo de ataque de la envolvente de soporte de sustain.",
        "Ajusta el tiempo de liberación de la envolvente de soporte de sustain.",
        "Ajusta la diferencia máxima permitida de reducción de ganancia entre canales.",
        // values
        "Muestra/oculta el panel de ajustes de lectura de valores.",
        "Muestra el nivel máximo de True Peak retenido.",
        "Muestra la correlación de fase estéreo actual (izquierda) y media (derecha).",
        "Muestra la sonoridad momentánea actual (izquierda) y máxima (derecha).",
        "Muestra la sonoridad a corto plazo actual (izquierda) y máxima (derecha).",
        "Muestra el rango de sonoridad. Los paréntesis indican una medición provisional.",
        "Muestra la sonoridad integrada.",
        " "
    };
}
