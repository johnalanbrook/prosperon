class Method
    def source(n=5)
        loc = source_location
        puts `sed -n "#{loc[1]},#{loc[1]+n}p;#{loc[1]+6}q" #{loc[0]}`
    end
end

class Music

    def initialize(mpath = "")
        load(mpath);
    end


    def load(mpath)
        #@music = load(path);
    end

    def play

    end
end

class Sound
    def initialize(path = "")
        load(path);
    end

    def load(path)
        #@sound =
    end

    def play

    end
end

def checknewer
    $maketimes ||= {}
    file = caller[0].split(":")[0]
    if $maketimes[file].nil?
        $maketimes[file] = `stat --printf '%Y' #{file}`
        return
    end

    newtime = `stat --printf '%Y' #{file}`
    if newtime > $maketimes[file]
        load(file)
    end
end

def set_renderms(val)
    settings_cmd(0, val);
end

def set_updatems(val)
    settings_cmd(1, val);
end

def set_physms(val)
    settings_cmd(2, val);
end