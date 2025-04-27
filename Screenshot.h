#pragma once

//  Copyright 2015 by Joseph Forgion
//  This file is part of VCC (Virtual Color Computer).
//
//  VCC (Virtual Color Computer) is free software: you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  VCC (Virtual Color Computer) is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with VCC. If not, see <http://www.gnu.org/licenses/>.
//

#include "IDisplay.h"

namespace Screenshot
{
    enum
    {
        OK,
        ERR_NOUSERPROFILE,
        ERR_PATHTOOLONG,
        ERR_FILECLASH,
        ERR_NOLOCK,
        ERR_NOUNLOCK,
        ERR_NOSURFACE,
        ERR_NOAREA,
        ERR_WRITEPNG,
    };

    int Snap(VCC::IDisplay* display);
}


