@echo off

for /F "eol=# tokens=1,2,3* delims= " %%i in (shader_list.txt) do call .\dxc_bin\compile_single_shader.bat %%i %%j %%k

pause