/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "app.h"
#include "debug.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;


App::OptionGroup special_options () 
{
  using namespace App;
  OptionGroup specific_options = OptionGroup ("My options")
    + Option ("specific", "some description")
    +  Argument ("arg")

    + Option ("special", "more text")
    +   Argument ("x").type_image_in()
    +   Argument ("y").type_file();

  return specific_options;
}



void usage () {

  AUTHOR = "Joe Bloggs";

  VERSION[0] = 1;
  VERSION[1] = 4;
  VERSION[2] = 3;

  COPYRIGHT = "whatever you want";

  DESCRIPTION 
    + "this is used to test stuff. I need to write a lot of stuff here to pad this out and check that the wrapping functionality works as advertised... Seems to do an OK job so far. Wadaya reckon?"
    + "some more details here.";

  ARGUMENTS
    + Argument ("arg", "an argument")
    + Argument ("sef", "some strange value").type_float (0.0, 1.0, 10.0);

  OPTIONS
    + Option ("poo", "its description")
    + Argument ("arg1").type_integer (0,1,10)
    + Argument ("arg2")

    + Option ("crap", "another description")
    + Argument ("stuff").type_float (-1.0, 0.0, 1.0)

    + special_options();
}




typedef cfloat T;

void run ()
{
  Options opt = get_options ("poo");
  if (opt.size()) {
    VAR (int (opt[0][0]));
    VAR (opt[0][1]);
  }

  VAR (argument[0]);
  
}

