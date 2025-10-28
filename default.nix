let
  pkgs = import <nixpkgs> { };
in

{
  # Native build for current system.
  native = pkgs.callPackage ./package.nix { };

  # Static build for remarkable2 that uses musl as libc.
  remarkable2 = pkgs.pkgsCross.remarkable2.pkgsStatic.callPackage ./package.nix { };
}
