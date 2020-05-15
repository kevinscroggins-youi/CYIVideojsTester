#!/usr/bin/env ruby

$temp_directory = "temp_widget_data"
$resigned_widget_file_name_suffix = "New"
$widget_file_path = ""
$widget_file_extension = "wgt"
$widget_config_file_name = "config.xml"
$widget_name_regex = /<[ \t]*name[ \t]*>([^<>]+)<[ \t]*\/[ \t]*name[ \t]*>/
$overwrite = true
$verbose = true
$version = "1.0.0"
$date_modified = "November 4, 2019"
$author = "Kevin Scroggins"
$email = "kevin.scroggins@youi.tv"

require "open3"
require "ostruct"
require "io/console"
require "fileutils"

def gem_available(name)
	return Gem::Specification.find_by_name(name)
rescue Gem::LoadError
	return false
rescue
	return Gem.available?(name)
end

begin
	require "Win32API"

	def get_char()
		Win32API.new("crtdll", "_getch", [], "L").Call
	end
rescue LoadError
	def get_char()
		state = `stty -g`
		`stty raw -echo -icanon isig`

		STDIN.getc.chr
	ensure
		`stty #{state}`
	end
end

# check ruby version
$ruby_version = ""
$ruby_version_regex = /ruby (([0-9]+)\.([0-9]+)(.([0-9]+))?)(p([0-9]+))?/
stdout, stderr, status = Open3.capture3("ruby --version")

stdout.each_line do |line|
	if line.to_s.empty?
		next
	end

	ruby_version_data = $ruby_version_regex.match(line)

	if ruby_version_data
		$ruby_version = ruby_version_data[1]
		$ruby_patch = ruby_version_data[7]

		if ruby_version_data[2].to_i < 2 or ruby_version_data[3].to_i < 5
			puts "WARNING: Ruby version appears to be old, consider updating the latest version of Ruby."
			$insert_spacer = true
		end

		break
	end
end

# check required gems
$auto_install_gems = true

missing_gems = []

required_gems = ["rainbow", "rubyzip"]

rainbow_available = true

required_gems.each do |gem_name|
	if !gem_available(gem_name)
		missing_gems.push(gem_name)

		if gem_name == "rainbow"
			rainbow_available = false
		end
	end
end

if rainbow_available
	require "rainbow"
end

if !missing_gems.empty?
	if rainbow_available
		print Rainbow("ERROR: Missing the following gem" + (missing_gems.length == 1 ? "" : "s") + ": \"" + missing_gems.join(", ") + "\"" + ($auto_install_gems ? "." : ", would you like to install " + (missing_gems.length == 1 ? "it" : "them") + "? (")).red + Rainbow("y/n").magenta + Rainbow("): ").red
	else
		print "ERROR: Missing the following gem" + (missing_gems.length == 1 ? "" : "s") + ": \"" + missing_gems.join(", ") + "\"" + ($auto_install_gems ? "." : ", would you like to install " + (missing_gems.length == 1 ? "it" : "them") + "? (y/n): ")
	end

	input = nil

	if !$auto_install_gems
		input = get_char()
		puts input
	else
		puts ""
	end

	if $auto_install_gems or input.upcase == "Y"
		missing_gems.each_index do |gem_index|
			gem_name = missing_gems[gem_index]

			puts ""
			if rainbow_available
				puts Rainbow("Installing ").green + Rainbow(gem_name).cyan + Rainbow(" gem (").green + Rainbow((gem_index + 1).to_s).cyan + Rainbow(" / ").green + Rainbow(missing_gems.length).cyan + Rainbow(")...").green
			else
				puts "Installing " + gem_name + " gem (" + (gem_index + 1).to_s + " / " + missing_gems.length.to_s + ")..."
			end

			gem_installed_regex = /Successfully installed #{gem_name}(-(([0-9]+)\.([0-9]+)(.([0-9]+))?))?/

			output = ""

			Open3.popen3("gem install " + gem_name) { |stdin, stdout, stderr, wait_thr|
				pid = wait_thr.pid

				stdin.close

				stdout.each_line { |line|
					puts line
					output += line
				}

				stdout.close
				stderr.close
				exit_status = wait_thr.value
			}

			gem_installed = false

			output.each_line do |line|
				if gem_installed_regex.match(line)
					gem_installed = true
					break
				end
			end

			if !gem_installed
				puts ""
				if rainbow_available
					puts Rainbow("Failed to install \"").yellow + Rainbow(gem_name).cyan + Rainbow(" gem, trying again with sudo.").yellow
				else
					puts "Failed to install \"" + gem_name + " gem, trying again with sudo."
				end

				system("sudo gem install " + gem_name)
			end
		end

		puts ""

		if rainbow_available
			puts Rainbow("Please re-execute your command!").green
		else
			puts "Please re-execute your command!"
		end
	end

	exit(1)
end

require "zip"

# operating system detection
$windows = false
$linux = false
$solaris = false
$osx = false
$unsupported_os = false

case RbConfig::CONFIG["host_os"]
	when /mswin|windows|mingw/i
		$operating_system = "windows"
		$windows = true
	when /linux|arch/i
		$operating_system = "linux"
		$linux = true
	when /sunos|solaris/i
		$operating_system = "solaris"
		$solaris = true
		$unsupported_os = true
	when /darwin/i
		$operating_system = "osx"
		$osx = true
	else
		$operating_system = "unknown"
		$unsupported_os = true
end

def join_paths(*args)
	if args.nil? or args.length == 0
		return nil
	end

	path = nil

	args.each do |arg|
		if path.nil?
			path = arg
		else
			path = File.join(path, arg)
		end
	end

	if $windows
		path = path.gsub(File::SEPARATOR, File::ALT_SEPARATOR || File::SEPARATOR)
	end

	return path
end

if ARGV.length != 0
	$widget_file_path = ARGV[0].to_s
end

if $widget_file_path.to_s.empty?
	puts Rainbow("ERROR: Missing widget file path, please specify the path to your widget file as an argument or hard-code it in the script.").red
	exit(1)
end

if !File.file?($widget_file_path)
	puts Rainbow("ERROR: Widget file does not exist or is not a file.").red
	exit(1)
end

if $verbose
	puts Rainbow("Tizen Widget Re-Signing Script Version: ").green + Rainbow($version.to_s).cyan + Rainbow(" Modified: ").green + Rainbow($date_modified.to_s).cyan
	puts Rainbow("Widget File Path: ").green + Rainbow($widget_file_path).cyan
	puts Rainbow("Temporary Directory: ").green + Rainbow($temp_directory).cyan
	puts Rainbow("Tizen Widget Config File Name: ").green + Rainbow($widget_config_file_name).cyan
	puts Rainbow("Ruby Version: ").green + Rainbow($ruby_version + ($ruby_patch.to_s.empty? ? "" : "p" + $ruby_patch)).cyan
	puts Rainbow("Operating System: ").green + Rainbow($operating_system).cyan
	puts ""
end

if File.directory?($temp_directory)
	if $verbose
		puts Rainbow("Removing temporary directory...").green
		puts ""
	end

	FileUtils.remove_dir($temp_directory);
end

if $verbose
	puts Rainbow("Creating temporary directory...").green
	puts ""
end

FileUtils.mkdir_p($temp_directory)

if $verbose
	puts Rainbow("Extracting widget file contents to temporary directory...").green
end

Zip::File.open($widget_file_path) do |widget_file|
	widget_file.each do |widget_file_entry|
		if $verbose
			puts Rainbow(" + ").green + Rainbow(widget_file_entry.name).cyan
		end

		widget_file_entry_path = join_paths($temp_directory, widget_file_entry.name)
		widget_file_entry_subdirectory = File.dirname(widget_file_entry_path)

		if !Dir.exist?(widget_file_entry_subdirectory)
			puts Rainbow("   - Creating subdirectory \"").green + Rainbow(widget_file_entry_subdirectory).cyan + Rainbow("\"...").green

			FileUtils.mkdir_p(widget_file_entry_subdirectory)
		end

		widget_file_entry.extract(widget_file_entry_path)
	end
end

# read widget name
if $verbose
	puts ""
	puts Rainbow("Detecting Widget name from config file...").green
end

$widget_name = ""
$new_widget_file_path = ""
$widget_config_file_path = join_paths($temp_directory, $widget_config_file_name)

if !File.file?($widget_config_file_path)
	puts Rainbow("ERROR: Tizen widget config file does not exist or is not a file.").red
	exit(1)
else
	File.open($widget_config_file_path, "r") do |widget_config_file|
		widget_config_file.each_line do |line|
			if line.empty?
				next
			end

			widget_name_data = $widget_name_regex.match(line)

			if widget_name_data
				$widget_name = widget_name_data[1]
				$new_widget_file_path = join_paths($temp_directory, $widget_name + "." + $widget_file_extension)
			end

			if !$widget_name.to_s.empty?
				break
			end
		end
	end
end

if $widget_name.to_s.empty?
	puts ""
	puts Rainbow("ERROR: Missing or empty Tizen widget name, is the name element correctly specified in your widget config file?").red
	exit(1)
end

if $verbose
	puts ""
	puts Rainbow("Widget Name: ").green + Rainbow($widget_name).cyan
	puts ""
	puts Rainbow("Re-signing Tizen widget...").green
end

system("tizen package --type wgt -- " + $temp_directory)

if !File.file?($new_widget_file_path)
	puts ""
	puts Rainbow("ERROR: Failed to re-sign Tizen widget or widget file does not exist with expected file name at path: \"").red + Rainbow($new_widget_file_path).yellow + Rainbow("\".").red
	exit(1)
end

# TODO: add dynamic tizen cli detection, this will not work on Windows or if environment variables are not correctly set
# TODO: add support for signing with a local certificate
# TODO: verify that widget contains signatrue files

if $overwrite
	if $verbose
		puts ""
		puts Rainbow("Replacing original Tizen widget file...").green
	end

	if File.file?($widget_file_path)
		FileUtils.remove_file($widget_file_path)
	end

	FileUtils.move($new_widget_file_path, $widget_file_path)
else
	moved_widget_file_path = join_paths(File.dirname($widget_file_path), $widget_name + $resigned_widget_file_name_suffix + "." + $widget_file_extension)

	if $verbose
		puts ""
		puts Rainbow("Moving new Tizen widget file from \"").green + Rainbow($new_widget_file_path).cyan + Rainbow("\" to \"").green + Rainbow(moved_widget_file_path).cyan + Rainbow("\"...").green
	end

	if File.file?(moved_widget_file_path)
		FileUtils.remove_file(moved_widget_file_path)
	end

	FileUtils.move($new_widget_file_path, moved_widget_file_path)
end

if $verbose
	puts ""
	puts Rainbow("Removing temporary directory...").green
end

FileUtils.remove_dir($temp_directory);

if $verbose
	puts ""
	puts Rainbow("Done!").green
end
