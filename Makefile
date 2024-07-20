.PHONY: build kill debug release

build:
	cl /O2 /GL /Gw .\dual-key-remap.c /link user32.lib shell32.lib /ENTRY:mainCRTStartup

kill:
	@taskkill /f /im "dual-key-remap.exe" || echo dual-key-remap is not running

debug:
	$(MAKE) kill
	$(MAKE) build
	set DEBUG=1
	.\dual-key-remap.exe

release:
	$(MAKE) kill
	$(MAKE) build
	powershell .\release.ps1
