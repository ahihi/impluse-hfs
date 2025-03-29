{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      eachSystem = nixpkgs.lib.genAttrs [
        "aarch64-darwin"
        "x86_64-darwin"
      ];
    in {
      overlays = {
        default = final: prev:
          let
            system = prev.stdenv.hostPlatform.system;
            pkgs = import nixpkgs { inherit system; };
          in {
            impluse-hfs = final.callPackage ./default.nix {
              inherit (pkgs)
                apple-sdk
                xcbuildHook;
            };
          };
      };
      packages = eachSystem (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [self.overlays.default];
          };
        in {
          default = pkgs.impluse-hfs;
        }
      );
    };
}
