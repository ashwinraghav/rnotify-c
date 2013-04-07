current_dir = File.dirname(__FILE__)
log_level                :info
log_location             STDOUT
node_name                "ashwinraghav"
client_key               "#{current_dir}/ashwinraghav.pem"
validation_client_name   "uva-validator"
validation_key           "#{current_dir}/uva-validator.pem"
chef_server_url          "https://api.opscode.com/organizations/uva"
cache_type               'BasicFile'
cache_options( :path => "#{ENV['HOME']}/.chef/checksums" )
cookbook_path            ["#{current_dir}/../cookbooks"]
