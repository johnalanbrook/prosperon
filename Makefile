debug: FORCE
	meson setup build_dbg -Dbuildtype=debugoptimized
	meson compile -C build_dbg

release: FORCE
	meson setup -Dbuildtype=release -Db_lto=true -Db_ndebug=true -Db_pgo=use build_release
	meson compile -C build_release

sanitize: FORCE
	meson setup -Db_sanitize=address -Db_sanitize=memory -Db_sanitize=leak -Db_sanitize=undefined build_sani
	meson compile -C build_sani

small: FORCE
	meson setup -Dbuildtype=minsize -Db_lto=true -Db_ndebug=true -Db_pgo=use build_small
	meson compile -C build_small

web: FORCE
	meson setup -Deditor=false -Dbuildtype=minsize -Db_lto=true -Db_ndebug=true --cross-file emscripten.cross build_web
	meson compile -C build_web

crosswin: FORCE
	meson setup -Dbuildtype=debugoptimized --cross-file mingw32.cross build_win
	meson compile -C build_win

FORCE:
