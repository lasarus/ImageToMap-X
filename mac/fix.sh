#!/bin/bash
export INT=install_name_tool

echo Fixing...
$INT -change /usr/local/lib/libgtk-quartz-2.0.0.dylib @executable_path/libgtk-quartz-2.0.0.dylib $1
$INT -change /usr/local/lib/libgdk-quartz-2.0.0.dylib @executable_path/libgdk-quartz-2.0.0.dylib $1
$INT -change /usr/local/lib/libatk-1.0.0.dylib @executable_path/libatk-1.0.0.dylib $1
$INT -change /usr/local/lib/libgio-2.0.0.dylib @executable_path/libgio-2.0.0.dylib $1
$INT -change /usr/local/lib/libpangocairo-1.0.0.dylib @executable_path/libpangocairo-1.0.0.dylib $1
$INT -change /usr/local/lib/libgdk_pixbuf-2.0.0.dylib @executable_path/libgdk_pixbuf-2.0.0.dylib $1
$INT -change /usr/local/lib/libpango-1.0.0.dylib @executable_path/libpango-1.0.0.dylib $1
$INT -change /usr/local/lib/libcairo.2.dylib @executable_path/libcairo.2.dylib $1
$INT -change /usr/local/lib/libgobject-2.0.0.dylib @executable_path/libgobject-2.0.0.dylib $1
$INT -change /usr/local/lib/libgmodule-2.0.0.dylib @executable_path/libgmodule-2.0.0.dylib $1
$INT -change /usr/local/lib/libgthread-2.0.0.dylib @executable_path/libgthread-2.0.0.dylib $1
$INT -change /usr/local/lib/libglib-2.0.0.dylib @executable_path/libglib-2.0.0.dylib $1
$INT -change /usr/local/lib/libintl.8.dylib @executable_path/libintl.8.dylib $1
echo Done
