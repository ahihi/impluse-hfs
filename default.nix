{
  lib,
  stdenv,
  fetchFromGitHub,
  apple-sdk,
  xcbuildHook,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "impluse-hfs";
  version = "e14dc33";
  
  src = ./.;

  nativeBuildInputs = [
    apple-sdk
    xcbuildHook
  ];
  buildInputs = [];

  xcbuildFlags = [
    "-configuration"
    "Release"
    "OTHER_CFLAGS=\"-fmodules-cache-path=ClangModuleCache\""
  ];
  __structuredAttrs = true;

  preConfigurePhase = ''
    mkdir -p ClangModuleCache
  '';

  installPhase = ''
    runHook preInstall
    ls -alpR
    mkdir -p $out/bin
    mv Products/Release/impluse-hfs $out/bin/
    runHook postInstall
  '';
  
  meta = with lib; {
    description = "A tool for converting HFS (Mac OS Standard) volumes to HFS+ (Mac OS Extended) format";
    homepage = "https://github.com/boredzo/impluse-hfs";
    license = licenses.bsd3;
    platforms = platforms.darwin;
    maintainers = [ ];
    mainProgram = "impluse";
  };
})
