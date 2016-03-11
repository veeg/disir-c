# encoding: UTF-8

require 'optparse'

class GitHook

  attr_reader :errors
  def initialize
    @errors = 0
  end

  def parse! args
    opts = OptionParser.new do |opt|
      opt.on('-v', '--verbose') do |verbose|
        @verbose = verbose
      end
      opt.on('-d', '--debug') do |debug|
        @debug = debug
      end
    end

    opts.parse! args
    @args = args
  end

  def error msg
    $stderr.puts "[POLICY]: #{self.class} - #{msg}"
    @errors += 1
  end

  def debug msg
    $stderr.puts msg if @debug
  end

  def verbose msg
    $stderr.puts msg if @verbose
  end

  def self.git_root
    git_root = `git rev-parse --show-toplevel`
    git_root.strip!
    if $?.exitstatus != 0
      $stderr.puts "Did not run as part of a git repository."
      exit 1
    end
    git_root
  end

  def self.hook args
    a = self.new
    a.parse! args

    if not a.respond_to? :run
      $stderr.puts "GitHook has not implemented :run"
    end

    a.verbose "Running GitHook: #{a.class}"
    a.run
    if a.errors > 0
      exit 1
    end
  end

  def self.invoke_hooks name, args, verbose = false, debug = false
    root = git_root
    path = "#{root}/.git/hooks/"
    Dir.foreach path do |entry|
      next if entry.start_with? '.'
      next if File.directory? entry
      next if entry.end_with? '.sample'
      next if not entry.start_with? name
      next if entry.eql? name

      # Run subhook
      cmd = "#{root}/.git/hooks/#{entry} "
      cmd += " --verbose" if verbose
      cmd += " --debug" if debug
      cmd += args.join ' '
      output = `#{cmd}`
      if $?.exitstatus != 0
        $stderr.puts output
        exit 1
      end
    end
  end
end

# Monkey patch File to check if file is Binary.
def File.binary? name
  open name do |f|
    while (b=f.read(256)) do
      return true if b[ "\0"]
    end
  end
  false
end
