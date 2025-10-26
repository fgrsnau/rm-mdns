{
  lib,
  stdenv,

  meson,
  ninja,
}:

stdenv.mkDerivation {
  pname = "rm-mdns";
  version = "1.0.0";

  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [
      ./mdns.h
      ./meson.build
      ./rm-mdns.c
    ];
  };

  nativeBuildInputs = [
    meson
    ninja
  ];
}
