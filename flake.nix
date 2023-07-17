{
  description = "A flake for building QEMU";

  inputs.nixpkgs.url = "nixpkgs/nixos-23.05";

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in 
  {

    devShells.${system}.default = pkgs.mkShell.override { } {
      buildInputs = [
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

        pkgs.flex
        pkgs.bison
      ];
    };
  };
}
