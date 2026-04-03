{
  description = "devsink - hardware device sink interfaces (stbsink, lcdsink)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    # The shared lib directory lives in the parent testbed monorepo.
    # It is not a flake, so flake = false tells Nix to treat it as plain source.
    testbedLib = {
      url  = "path:../lib";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, testbedLib }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };

    devsink = pkgs.stdenv.mkDerivation {
      pname = "devsink";
      version = "0.2.0-dev";

      src = ./src;

      nativeBuildInputs = with pkgs; [ meson ninja pkg-config ];
      buildInputs    = with pkgs; [ zeromq glfw ];

      postPatch = ''
        # The lib/ symlinks point outside the git tree and are broken in the
        # Nix store.  Replace the whole directory with a single symlink into
        # the locked testbedLib store path.
        rm -rf lib
        ln -s ${testbedLib} lib

        # Enable installation of both executables (meson defaults to false).
        substituteInPlace stbsink/meson.build \
          --replace "install : false" "install : true"
        substituteInPlace lcdsink/meson.build \
          --replace "install : false" "install : true"

        # Rewrite the compile-time path macros to point at the final store
        # location instead of meson.project_source_root().
        substituteInPlace lcdsink/meson.build \
          --replace \
            "'SRC_ROOT': meson.project_source_root()" \
            "'SRC_ROOT': '${placeholder "out"}/share/devsink'"

        substituteInPlace stbsink/meson.build \
          --replace \
            "join_paths(meson.project_source_root(), 'lib', 'kato')" \
            "'${placeholder "out"}/share/devsink/kato'"
      '';

      postInstall = ''
        # ── lcdsink runtime data ──────────────────────────────────────────
        # Shaders (loaded at runtime via LCDSINK_SRC_ROOT/lcdsink/*.{vert,frag})
        install -Dm644 $src/lcdsink/direct.vert \
          $out/share/devsink/lcdsink/direct.vert
        install -Dm644 $src/lcdsink/direct.frag \
          $out/share/devsink/lcdsink/direct.frag

        # Background calibration image
        install -Dm644 $src/lcdsink/data/grid_gray16_1620_2560.png \
          $out/share/devsink/lcdsink/data/grid_gray16_1620_2560.png

        # ── stbsink / kato font ───────────────────────────────────────────
        # Font loaded at runtime via KATO_DIR/ProggyClean.ttf
        install -Dm644 ${testbedLib}/kato/ProggyClean.ttf \
          $out/share/devsink/kato/ProggyClean.ttf
      '';
    };
  in {
    # ── Packages ────────────────────────────────────────────────────────────
    packages.${system} = {
      default = devsink;
      inherit devsink;
    };

    # ── NixOS module ────────────────────────────────────────────────────────
    nixosModules.default = { config, lib, pkgs, ... }:
    let
      cfg = config.services.devsink;
    in {
      options.services.devsink = {
        stbsink = {
          enable = lib.mkEnableOption "stbsink dummy modulator sink";
          user   = lib.mkOption { type = lib.types.str; default = "devsink"; };
          group  = lib.mkOption { type = lib.types.str; default = "devsink"; };
        };
        lcdsink = {
          enable  = lib.mkEnableOption "lcdsink LCD SLM sink";
          user    = lib.mkOption { type = lib.types.str; default = "devsink"; };
          group   = lib.mkOption { type = lib.types.str; default = "devsink"; };
          display = lib.mkOption {
            type        = lib.types.str;
            default     = ":0";
            description = "X11 DISPLAY for OpenGL output.";
          };
        };
      };

      config = lib.mkMerge [
        (lib.mkIf cfg.stbsink.enable {
          users.users.${cfg.stbsink.user}   = { isSystemUser = true; group = cfg.stbsink.group; };
          users.groups.${cfg.stbsink.group} = {};

          systemd.services.stbsink = {
            description = "devsink stbsink dummy modulator";
            wantedBy    = [ "multi-user.target" ];
            after       = [ "network.target" ];
            serviceConfig = {
              ExecStart = "${devsink}/bin/stbsink";
              User      = cfg.stbsink.user;
              Group     = cfg.stbsink.group;
              Restart   = "on-failure";
            };
          };
        })

        (lib.mkIf cfg.lcdsink.enable {
          users.users.${cfg.lcdsink.user}   = { isSystemUser = true; group = cfg.lcdsink.group; };
          users.groups.${cfg.lcdsink.group} = {};

          systemd.services.lcdsink = {
            description = "devsink lcdsink LCD SLM";
            wantedBy    = [ "multi-user.target" ];
            after       = [ "graphical.target" ];
            environment.DISPLAY = cfg.lcdsink.display;
            serviceConfig = {
              ExecStart = "${devsink}/bin/lcdsink";
              User      = cfg.lcdsink.user;
              Group     = cfg.lcdsink.group;
              Restart   = "on-failure";
            };
          };
        })
      ];
    };

    # ── Dev shell (unchanged) ────────────────────────────────────────────────
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        gcc
        pkg-config
        glfw
        ninja
        meson
        zeromq
        cppzmq
      ];
    };
  };
}
