let
  pkgs = import <nixpkgs> { };
in

{
  native = pkgs.callPackage ./package.nix { };

  remarkable2 =
    let
      pkgsRemarkable2 = pkgs.pkgsCross.remarkable2;
    in
    pkgsRemarkable2.callPackage ./package.nix {
      stdenv = pkgsRemarkable2.stdenvAdapters.makeStatic pkgsRemarkable2.stdenv;
    };
}
