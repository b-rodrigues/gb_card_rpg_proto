{
  description = "Nix Game Boy LLM Development Kit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        gbdk = pkgs.stdenv.mkDerivation rec {
          pname = "gbdk";
          version = "4.3.0";

          src = pkgs.fetchzip {
            url = "https://github.com/gbdk-2020/gbdk-2020/releases/download/${version}/gbdk-linux64.tar.gz";
            sha256 = "0slw2ag8ljgcb6v8qz35f3k3zm8y9nc0j451cgnval7q086ar5xp";
          };

          nativeBuildInputs = [ pkgs.autoPatchelfHook ];
          buildInputs = [ pkgs.stdenv.cc.cc.lib pkgs.zlib ];

          installPhase = ''
            mkdir -p $out
            cp -r * $out/
          '';

          meta = with pkgs.lib; {
            description = "GBDK-2020 Game Boy Development Kit";
            homepage = "https://github.com/gbdk-2020/gbdk-2020";
            license = licenses.mit;
            platforms = platforms.linux;
          };
        };

        pyboy = pkgs.python3.pkgs.buildPythonPackage rec {
          pname = "pyboy";
          version = "2.7.0";

          format = "wheel";

          src = pkgs.fetchurl {
            url = "https://files.pythonhosted.org/packages/51/da/ce77683a235cbbf797c8eab25bd6dceabfd2c5109d75e06abbf7b27ff174/pyboy-2.7.0-cp314-cp314-manylinux2014_x86_64.manylinux_2_17_x86_64.manylinux_2_28_x86_64.whl";
            sha256 = "sha256-ivS1WtgnCzawDo0fnThu7Of3I1EYcuN+8twdR1bQnfc=";
          };

          nativeBuildInputs = [
            pkgs.autoPatchelfHook
          ];

          propagatedBuildInputs = with pkgs.python3.pkgs; [
            numpy
          ];

          pythonImportsCheck = [ "pyboy" ];

          # The wheel's METADATA declares pysdl2/pysdl2-dll, but both are
          # guarded by try/except ImportError in PyBoy and unnecessary for
          # headless (window="null") use; skip the runtime deps check.
          dontCheckRuntimeDeps = true;

          doCheck = false;

          meta = with pkgs.lib; {
            description = "Game Boy emulator written in Python";
            homepage = "https://github.com/Baekalfen/PyBoy";
            license = licenses.lgpl3Only;
            platforms = platforms.linux;
          };
        };
      in
      {
        packages.gbdk = gbdk;

        devShells.default = pkgs.mkShell {
          name = "gb-dev-shell";

          GBDKDIR = "${gbdk}/";
          GBDK_HOME = "${gbdk}/";

          buildInputs = [
            gbdk
            pkgs.rgbds
            pkgs.sameboy
            pkgs.mgba
            pkgs.gnumake
            pkgs.git
            pkgs.xvfb-run
            pkgs.imagemagick
            pkgs.python3
            pkgs.python3Packages.pillow
            pyboy
          ];

          shellHook = ''
            export GBDKDIR="${gbdk}/"
            export GBDK_HOME="${gbdk}/"
          '';
        };
      }
    );
}
