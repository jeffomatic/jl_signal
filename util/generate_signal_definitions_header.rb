require 'erubis'

local_path = File.dirname(__FILE__);
signal_class_template = Erubis::Eruby.new(File.open(File.join(local_path, 'signal_class_template.erb'), 'r').read)
header_template = Erubis::Eruby.new(File.open(File.join(local_path, 'header_template.erb'), 'r').read)

# Render each signal class
signal_classes = (0..8).map do |arg_count|
  binding = { arg_count: arg_count }.merge(
    if arg_count == 0
      {
        template_signature: 'typename _NoParam = void',
        arg_type_list: 'void',
        arg_signature: 'void',
        arg_list: '',
      }
    else
      {
        template_signature: (1..arg_count).map{ |i| "typename _P#{i}" }.join(', '),
        arg_type_list: (1..arg_count).map{ |i| "_P#{i}" }.join(', '),
        arg_signature: (1..arg_count).map{ |i| "_P#{i} p#{i}" }.join(', '),
        arg_list: (1..arg_count).map{ |i| "p#{i}" }.join(', '),
      }
    end
  )
   
  # Apply the binding to the template, and truncate any empty function calls.
  signal_class_template.result(binding).gsub(/\(\s+\)/, '()')
end

# Output the whole header
puts header_template.result(:signal_classes => signal_classes).strip