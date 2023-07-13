{
  description = "A flake for building QEMU + QFlex";

  inputs.nixpkgs.url = "nixpkgs/nixos-23.05";

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
        pkgs.python3
        pkgs.git
		    pkgs.pbzip2
		    pkgs.cmake
		    pkgs.glibc
		    pkgs.boost170
        pkgs.libdwarf

        pkgs.flex
        pkgs.bison
      ];
    };
  };
}
