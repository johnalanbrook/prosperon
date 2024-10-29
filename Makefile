debug: FORCE
	meson setup build_dbg -Dbuildtype=debugoptimized
	meson compile -C build_dbg

release: FORCE
	meson setup -Dbuildtype=release -Db_lto=true -Db_lto_threads=4 -Db_ndebug=true -Db_pgo=use build_release
	meson compile -C build_release

sanitize: FORCE
	meson setup -Db_sanitize=address -Db_sanitize=memory -Db_sanitize=leak -Db_sanitize=undefined build_sani
	meson compile -C build_sani

FORCE:
