require File.expand_path("../lib/c_helper/version", __FILE__)

Gem::Specification.new do |s|
  s.name = "c_helper"
  s.version = "1.0.0"
  s.date = "2019-04-05"
  s.summary = "Helper code in C to speed things up"
  s.authors = ["Andrew MacNamara", "Kamlesh Gokal"]
  s.email = ["amacnamara@hl.com", "kamleshg@magenic.com"]
  s.extensions = ["ext/c_helper/extconf.rb"]
  s.files = [
    'ext/c_helper/backsolve_cf.c',
    'lib/c_helper.rb',
    'lib/c_helper/version.rb'
  ]
  s.require_paths = ['lib']
end