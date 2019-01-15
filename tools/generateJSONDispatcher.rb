
# Generate the FHiCL document which configures the ots::JSONDispatcher class

# Note that in the case of the "prescale"
# parameters, if it is "nil", the
# function, their values will be taken from a separate file called
# JSONDispatcher.fcl, searched for via "read_fcl"



require File.join( File.dirname(__FILE__), 'ots_utilities' )

def generateJSONDispatcher(fragmentIDList, fragmentTypeList, port, prescale = nil)

 jsonConfig = String.new( "\
    json: {
      module_type: JSONDispatcher
      fragment_ids: %{fragment_ids}
      fragment_type_labels: %{fragment_type_labels} 
      port: %{onmonPort}" \
      + read_fcl("JSONDispatcher.fcl") \
      + "    }" )


    # John F., 1/21/14 -- before sending FHiCL configurations to the
    # EventBuilderMain and AggregatorMain processes, construct the
    # strings listing fragment ids and fragment types which will be
    # used by the WFViewer

    # John F., 3/8/14 -- adding a feature whereby the fragments are
    # sorted in ascending order of ID

    fragmentIDListString, fragmentTypeListString = "[ ", "[ "

    typemap = Hash.new

    0.upto(fragmentIDList.length-1) do |i|
      typemap[ fragmentIDList[i]  ] = fragmentTypeList[i]
    end

    fragmentIDList.sort.each { |id| fragmentIDListString += " %d," % [ id ] }
    fragmentIDList.sort.each { |id| fragmentTypeListString += "%s," % [ typemap[ id ] ] }

    fragmentIDListString[-1], fragmentTypeListString[-1] = "]", "]" 

  jsonConfig.gsub!(/\%\{fragment_ids\}/, String(fragmentIDListString))
  jsonConfig.gsub!(/\%\{fragment_type_labels\}/, String(fragmentTypeListString))
  jsonConfig.gsub!(/\%\{onmonPort\}/,String(port))

if ! prescale.nil?
  jsonConfig.gsub!(/.*prescale.*\:.*/, "prescale: %d" % [prescale])
end

  return jsonConfig
end

