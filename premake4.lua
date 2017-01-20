solution "Webhook_Bridge"	
	configurations { "Debug", "Release" }
	language "C++"

	--Project build target--
	project "Webhook_Bridge"
	targetdir ("bin/")
	--Change this--
	targetname ("Webhook_Bridge")
	---------------
	kind "ConsoleApp"
	--Compiler--
	--buildoptions { "-std=c++14" } --already done in newer compilers
	files { "src/**.h", "src/**.cpp" }
	--pchsource "DecoyEngine/src/stdafx.cpp"
	--pchheader "DecoyEngine/src/stdafx.h"
	--Linker--
	objdir("obj/")
	if os.is("windows") then
		includedirs { "src/", "include/" }
		libdirs { "lib/" }
		links { "" }
	end
	if os.is("linux") then		
		includedirs { "src/" }
		links { "curl", "mongoose" }
	end
	if os.is("macosx") then		
		includedirs { "src/", "include/" }
		links { "" }
	end	

	--Configurations-- 	
	configuration "Debug"
		defines { "_DEBUG", "DEBUG" }
		flags { "Symbols", "ExtraWarnings" }
	configuration "Release"
		defines { "NDEBUG" }
		flags { "OptimizeSpeed", "FloatFast", "EnableSSE2" }
		--other interesting flags:
		--FloatStrict
		--OptimizeSize
