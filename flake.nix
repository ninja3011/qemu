{
  description = "A flake for building QEMU + QFlex";

  inputs.nixpkgs.url = "nixpkgs/nixos-22.05";

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in 
  {
    devShells.${system}.default = pkgs.mkShell {
      buildInputs = [
        pkgs.gcc
        pkgs.ninja
        pkgs.glib
        pkgs.pkg-config
        pkgs.pixman
        pkgs.capstone
        pkgs.libslirp
        pkgs.libgcrypt
      ];
    };

    package.${system}.default = pkgs.stdenv.mkDerivation {
      name = "QEMU QFLex";
      src = ./.;
      buildInputs = [
        pkgs.gcc
        pkgs.ninja
        pkgs.glib
        pkgs.pkg-config
        pkgs.pixman
        pkgs.capstone
        pkgs.libslirp
      ];
      
      buildPhase = ''
        ./configure --target-list=aarch64-softmmu --disable-gtk --enable-sanitizers --enable-capstone && cd build && ninja && cd ..
      '';
    };

  };
}