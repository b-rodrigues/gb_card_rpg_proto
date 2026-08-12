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
          ];

          shellHook = ''
            export GBDKDIR="${gbdk}/"
            export GBDK_HOME="${gbdk}/"
          '';
        };
      }
    );
}
