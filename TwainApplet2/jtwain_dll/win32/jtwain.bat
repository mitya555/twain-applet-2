rem Need to set path in autoexec.bat
set path=F:\Distr\Borland\Bcc55\Bin;%path%
rem Control Panel -> System -> Advanced Tab -> Environment Variables
rem cd C:\Documents and Settings\Mitya\Desktop\uk.co.mmscomputing.device.twain\jtwain_dll\win32
bcc32 -w-par -tWD -I"F:\Distr\Borland\Bcc55\include" -L"F:\Distr\Borland\Bcc55\lib;F:\Distr\Borland\Bcc55\lib\psdk" -L"F:\Distr\Borland\Bcc55\lib" -e"jtwain.dll" *.cpp