require 'mkmf'

$LOCAL_LIBS << '' # add libraries needed for compilation here

if RUBY_PLATFORM =~ /darwin/
  # $LDFLAGS << '-framework AppKit'
end

create_header
create_makefile 'c_helper/c_helper'