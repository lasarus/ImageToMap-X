#!/bin/bash
export INT=i686-apple-darwin10-install_name_tool

echo Fixing...
$INT -change /Users/wildn00b/gtk/inst/lib/libgtk-quartz-2.0.0.dylib @executable_path/libgtk-quartz-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgdk-quartz-2.0.0.dylib @executable_path/libgdk-quartz-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libatk-1.0.0.dylib @executable_path/libatk-1.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgio-2.0.0.dylib @executable_path/libgio-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libpangocairo-1.0.0.dylib @executable_path/libpangocairo-1.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgdk_pixbuf-2.0.0.dylib @executable_path/libgdk_pixbuf-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libpango-1.0.0.dylib @executable_path/libpango-1.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libcairo.2.dylib @executable_path/libcairo.2.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgobject-2.0.0.dylib @executable_path/libgobject-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgmodule-2.0.0.dylib @executable_path/libgmodule-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libgthread-2.0.0.dylib @executable_path/libgthread-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libglib-2.0.0.dylib @executable_path/libglib-2.0.0.dylib $1
$INT -change /Users/wildn00b/gtk/inst/lib/libintl.8.dylib @executable_path/libintl.8.dylib $1
echo Done
