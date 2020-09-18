#!/usr/bin/env ruby
# © You.i TV Inc. 2000-2019. All rights reserved.

require 'fileutils'
require 'optparse'
require 'ostruct'

def GetIpAddress()
    ip = "127.0.0.1"

    begin
        TCPSocket.open('www.youi.tv', 80) do |sock|
            ip = sock.addr[3]
        end
    rescue
        puts "Unable to open socket, using localhost as default ip"
        puts "Unable to open socket, using 127.0.0.1 (localhost) as default IP address"
    end

    return ip
end

class GenerateOptions
    def self.parse(args)
        options = OpenStruct.new
        options.platform = nil
        options.build_directory = nil
        options.defines = { "YI_LOCAL_IP_ADDRESS" => GetIpAddress() }
        options.url_scheme = nil

        options.engine_hint = nil

        platformList = ["Android", "Bluesky2", "Bluesky4", "Ios", "Linux", "Osx", "Ps4", "Tizen-Nacl", "Tvos", "Uwp", "Vestel130", "Vestel211", "Vestel230", "Vs2017", "Webos3", "Webos4"]
        configurationList = ["Debug", "Release"]

        unless File.exist?(File.join("#{__dir__}", "CMakeLists.txt"))
            puts "ERROR: The directory '#{__dir__}' does not contain a CMakeLists.txt file."
            exit 2
        end

        options_parser = OptionParser.new do |opts|
            opts.banner = "Usage: generate.rb [options]"

            opts.separator ""
            opts.separator "Arguments:"

            opts.on("-p", "--platform PLATFORM", String,
                "(REQUIRED) The name of the platform to generate the project for.",
                "  Supported platforms: #{platformList}") do |platform|
                unless platformList.any? { |s| s.casecmp(platform)==0 }
                    puts "ERROR: \"#{platform}\" is an invalid platform."
                    puts opts
                    exit 1
                end

                options.platform = platform
            end

            opts.on("-g", "--generator GENERATOR", String,
                "The name of the generator to use.",
                "  (If omitted, the default generator for the host machine will be used.)",
                "  (When 'AndroidStudio' is specified for the generator and 'Android' is set for the platform, ",
                "   an Android Studio project will be generated.)",
                "Supported generators by platform:",
                "Android:",
                "  - AndroidStudio (default)",
                "OSX, iOS, and tvOS:",
                "  - Xcode (default)",
                "PS4:",
                "  - Visual Studio 11 [2012]",
                "  - Visual Studio 12 [2013]",
                "  - Visual Studio 14 [2015] (default)",
                "  Note: The PS4 SDK including the Visual Studio plugin for the selected generator must be installed.",
                "Tizen-NaCl:",
                "  - Eclipse CDT4 - Ninja (default if installed)",
                "  - Eclipse CDT4 - Unix Makefiles (default without ninja)",
                "VS2017 and UWP:",
                "  - Visual Studio 15 Win64 [2017] (default)",
                "Linux, Bluesky2, Bluesky4, Vestel130, Vestel211, Vestel230, WebOS3 and WebOS4:",
                "  - Ninja (default if installed)",
                "  - Unix Makefiles (default without ninja)",
) do |generator|
                options.generator = generator
            end

            opts.on("-b", "--build_directory DIRECTORY", String,
                "The directory in which the generated project files will be placed.") do |directory|
                options.build_directory = directory
            end

            opts.on("-d", "--define NAME=VALUE", String,
                "Add a defined variable and its value to pass along to CMake.") do |define_pair|

                key_value_pair = define_pair.split(/\s*=\s*/)
                if key_value_pair.length != 2
                    puts "Invalid format for -d: #{define_pair}"
                    puts opts
                    exit 1
                end

                options.defines[key_value_pair[0]] = key_value_pair[1]
            end

            opts.on("-c", "--config CONFIGURATION", String,
                "The configuration type #{configurationList} to send to the generator.",
                "  (This is only required for generators that do not support multiple configurations.)") do |config|
                if configurationList.any? { |s| s.casecmp(config)==0 }
                    options.defines["CMAKE_BUILD_TYPE"] = config
                else
                    puts "ERROR: \"#{config}\" is an invalid configuration type."
                    puts opts
                    exit 1
                end
            end

            opts.on("--url_scheme URL_SCHEME", String,
                "If included, the app will be able to be launched with deep links using this scheme.") do |url_scheme|
                options.url_scheme = url_scheme
            end

            opts.on("--youi_version ENGINE_HINT", String,
                "Can be set to a path (/path/to/5.0.0) or semantic version (5.0.0), and this project will generate against that version") do |engine_hint|
                options.engine_hint = engine_hint
            end

            opts.on_tail("-h", "--help", "Show usage and options list") do
                puts opts
                exit 1
            end
        end

        if args.count == 0
            puts options_parser
            exit 1
        end

        begin
            options_parser.parse!(args)
            mandatory = [:platform]
            missing = mandatory.select { |param| options[param].nil? }
            raise OptionParser::MissingArgument, missing.join(', ') unless missing.empty?

            if options.generator.nil?
                case options.platform
                when /android/i
                    options.generator = "AndroidStudio"
                when /osx|ios|tvos/i
                    options.generator = "Xcode"
                when /vs2017|UWP/i
                    options.generator = "Visual Studio 15 Win64"
                when /ps4/i
                    options.generator = "Visual Studio 15 ORBIS"
                when /Tizen-NaCl/i
                    ninja = system('ninja', [:out, :err] => File::NULL)
                    make = system('make', [:out, :err] => File::NULL)
                    if ninja != nil
                        options.generator = "Eclipse CDT4 - Ninja"
                    elsif make != nil
                        options.generator = "Eclipse CDT4 - Unix Makefiles"
                    else
                        puts "Could not find ninja or unix make. One of these generators must be installed to generate for Tizen-NaCl."
                        exit 1
                    end
                when /linux|bluesky2|bluesky4|vestel130|vestel211|vestel230|webos3|webos4/i
                    ninja = system('ninja', [:out, :err] => File::NULL)
                    make = system('make', [:out, :err] => File::NULL)
                    if ninja != nil
                        options.generator = "Ninja"
                    elsif make != nil
                        options.generator = "Unix Makefiles"
                    else
                        puts "Could not find ninja or unix make. One of these generators must be installed to generate for Linux, Bluesky2, Bluesky4, Vestel130, Vestel211, Vestel230, WebOS3, or WebOS4."
                        exit 1
                    end
                end
            end

            unless options.generator.match(/(Visual Studio)|Xcode|AndroidStudio/)
                unless options.defines.has_key?("CMAKE_BUILD_TYPE")
                    options.defines["CMAKE_BUILD_TYPE"] = "Debug"
                end
            end

            if (options.url_scheme)
                 options.defines["YI_BUNDLE_URL_SCHEME"] = "#{options.url_scheme}"
            end

            unless options.build_directory
                options.build_directory = File.expand_path(File.join(__dir__, "build", "#{options.platform.downcase}"))

                unless options.generator.match(/(Visual Studio)|Xcode|AndroidStudio/)
                    options.build_directory = File.join(options.build_directory, "#{options.defines["CMAKE_BUILD_TYPE"]}")
                end
            end


            return options
        rescue OptionParser::ParseError => e
            puts e
            puts ""
            puts options_parser
            exit 1
        end
    end

    def self.find_engine_dir_in_list(dirs)
        if dirs == nil || dirs.length == 0
            puts "ERROR: A non-empty list of directories must be passed to 'find_engine_dir_in_list'."
            abort
        end

        dirs.each { |d|
            config_filepath = File.absolute_path(File.join(d, "YouiEngineConfig.cmake"))
            if File.exist?(config_filepath)
                return d
            end

            # When in the engine repository, the YouiEngineConfig.cmake file doesn't exist at the root folder.
            # It's necessary to check for an alternative file.
            if File.exist?(File.absolute_path(File.join(d, "core", "CMakeLists.txt")))
                return d
            end
        }

        return nil
    end

    def self.sort_versions(vers)
        versions = []
        vers.each { |v|
            begin
                t = Gem::Version::new(v)
                versions.push(v)
            rescue
                # skip
            end
        }
        versions = versions.sort_by { |v| Gem::Version.new(v) }.reverse
    end

    def self.get_engine_dir(options)
        install_dir = File.join(ENV['HOME'], "youiengine")

        engine_dir = ""
        engine_dir = find_engine_dir_in_list([
            File.expand_path(__dir__),
            File.join(__dir__, ".."),
            File.join(__dir__, "..", ".."),
            File.join(__dir__, "..", "..", ".."),
            File.join(__dir__, "..", "..", "..", "..")
        ])
        if engine_dir != nil
            puts "WARNING: Found in engine directory. Will use this SDK, but please do out of SDK build!"
            return File.absolute_path(engine_dir)
        end

        if options.engine_hint != nil
            engine_dir = find_engine_dir_in_list([File.join(install_dir, options.engine_hint), options.engine_hint])
            if engine_dir != nil
                puts "Found engine directory #{engine_dir}"
                return File.absolute_path(engine_dir)
            else
                puts "ERROR: Passed youi_version variable #{options.engine_hint}, but could not find valid You.i Engine install"
                puts "Ensure that you have that version installed in $HOME/youiengine/, or the provided path is correct."
                abort
            end
        end

        versions = Dir.entries(install_dir)
        versions = sort_versions(versions).map! {|v| File.join(install_dir, v)}
        engine_dir = find_engine_dir_in_list(versions)

        unless engine_dir != nil
            puts "ERROR: Could not locate an installation of You.i Engine. Please install via youi-tv"
            puts "command line app, and try again, or pass the path to the installed SDK with the"
            puts "generate.rb option --youi_version=[arg]"
            abort
        end
        return File.absolute_path(engine_dir)
    end

    def self.create_command(options)
        case options.platform
        when /Android/i
            return GenerateOptions.create_android_command(options)
        end

        return GenerateOptions.create_cmake_command(options)
    end

    def self.create_android_command(options)
        engine_dir = GenerateOptions.get_engine_dir(options)
        source_dir = "#{__dir__}"
        build_dir = File.join("#{source_dir}", "build", "#{options.platform.downcase}")
        if !options.build_directory.nil?
            build_dir = options.build_directory
        end
        build_dir = File.absolute_path(build_dir)
        command = "cmake"
        command << " -DYI_OUTPUT_DIR=\"#{build_dir}\""

        cmake_defines = ""
        options.defines.each do |key,value|
            cmake_defines << " -D#{key}=\"#{value}\""
        end
        #Cleartext Enabled
        if !(cmake_defines.include? "YI_ENABLE_CLEARTEXT")
            cmake_defines << " -DYI_ENABLE_CLEARTEXT=ON"
        end
        command << "#{cmake_defines}"

        command << " -P \"#{File.join("#{engine_dir}", "cmake", "Modules", "YiGenerateAndroidStudioProject.cmake")}\""
        return command
    end

    def self.create_cmake_command(options)
        engine_dir = GenerateOptions.get_engine_dir(options)

        case options.platform
        when /ps4|uwp/i
            # PS4 and UWP use the built-in version of CMake, so we need to reference that
            # version instead of the standard one installed on the host machine.
            command = "\"#{File.absolute_path(File.join(engine_dir, "tools", "build", "cmake", "bin", "cmake.exe"))}\""
        else
            command = "cmake "
        end

        command << "\"-B#{options.build_directory}\" \"-H#{__dir__}\""

        unless options.generator.nil?
            command << " -G \"#{options.generator}\""
        end

        unless options.defines.has_key?("CMAKE_TOOLCHAIN_FILE")
            toolchain_platform = ""
            platform_sub_values = options.platform.split("-")

            platform_sub_values.each do |value|
                toolchain_platform << value.capitalize
            end

            case options.platform
            when /Tizen-NaCl/i
                if options.defines.has_key?("YI_ARCH")
                    tizen_valid_archs = ["armv7", "x86_64", "x86_32"]
                    if tizen_valid_archs.include?(options.defines["YI_ARCH"])
                        toolchain_platform << "-" << options.defines["YI_ARCH"]
                    else
                        puts "ERROR: Invalid YI_ARCH specified for Tizen-NaCl. Valid architectures: #{tizen_valid_archs}"
                        abort
                    end
                else
                    toolchain_platform << "-armv7"
                end
            end

            toolchain_subpath = File.join("cmake", "Toolchain", "Toolchain-" + toolchain_platform + ".cmake")

            toolchain_file = File.join(__dir__, toolchain_subpath)
            unless File.exist?(toolchain_file)
                toolchain_file = File.join(engine_dir, toolchain_subpath)
            end

            if File.exist?(toolchain_file)
                options.defines["CMAKE_TOOLCHAIN_FILE"] = toolchain_file
            else
                puts "ERROR: Default toolchain file not found for platform at path: #{toolchain_file}"
                abort
            end
        end

        cmake_defines = ""
        options.defines.each do |key,value|
            cmake_defines << " -D#{key}=\"#{value}\""
        end
        command << "#{cmake_defines}"

        return command
    end

end

options = GenerateOptions.parse(ARGV)
command = GenerateOptions.create_command(options)

puts "#=============================================="
puts "CMake Generator command line:"
puts "  #{command}"
puts ""
puts "Platform: #{options.platform}"

if !options.generator.nil?
    puts "Generator: #{options.generator}"
end
puts ""

if options.defines.length > 0
    puts "Defines:"
    options.defines.each do |key,value|
        puts "  - #{key}: #{value}"
    end
end

puts "#=============================================="

command_result = system(command)
if command_result == false || command_result == nil
    if command_result == nil
        puts "Generation failed -- could not execute cmake command. Ensure that cmake is installed and available in your PATH."
    end
    abort()
end
