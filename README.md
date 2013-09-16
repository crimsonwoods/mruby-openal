mruby-openal
====

_mruby-openal_ wrapped 'OpenAL'.

# How to build
----

1. edit your 'build_config.rb'.
2. run make command.

build_config.rb:

    conf.gem :github => 'crimsonwoods/mruby-openal', :branch => 'master'

    conf.cc do |cc|
      cc.include_paths << "/path/to/your/libopenal/include"
    end

    conf.linker do |linker|
      linker.libraries << %w(openal alut)
      linker.library_paths << "/path/to/your/libopenal/lib"
    end

# Sample code
----

Sample code is contained into 'samples' directory.

# License
----

MIT License

