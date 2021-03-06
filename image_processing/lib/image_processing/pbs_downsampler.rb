require 'fileutils'

module ImageProcessing
  class PbsDownsampler
    include FileUtils
    
    def initialize(config)
      @config = config
    end
    
    def go
      check_local_dirs
      
      originals_to_be_downsampled.each do |filename|
        downsample_file(filename)
      end
      puts "All files have been submitted!"
    end
    
  # private
    def check_local_dirs
      [@config.local_originals_dir, @config.local_downsamples_dir].each do |dir|
        mkdir_p(dir) unless Dir.exists?(dir)
      end
    end
     
    def downsample_file(filename)
      original   = File.join(@config.local_originals_dir,   filename)
      downsample = File.join(@config.local_downsamples_dir, filename)
      executable = File.join(Config::PROJECT_ROOT_DIR, "pbs_scripts", ENV['HOST'], "shrink_image")
      
      print "Submitting #{filename}..."
      `echo '#{executable} #{original} #{downsample} #{@config.downsample_ratio}' | qsub -N #{filename}`
      puts "done."
    end
  
    def originals
      Dir[File.join(@config.local_originals_dir, '*.bmp')].map {|f| File.basename f }
    end

    def downsamples
      Dir[File.join(@config.local_downsamples_dir, '*.bmp')].map {|f| File.basename f }
    end
    
    def originals_to_be_downsampled
      originals - downsamples
    end
  end
end
