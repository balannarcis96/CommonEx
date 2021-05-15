
add_subdirectory("nlohmann_json")
add_subdirectory("rapidXML")
add_subdirectory("Datastore")
add_subdirectory("fmt")
add_subdirectory("delegate")
add_subdirectory("mysql")

set_target_properties(nlohmann_json PROPERTIES FOLDER Contrib)
set_target_properties(rapidXML PROPERTIES FOLDER Contrib)
set_target_properties(Datastore_Lib PROPERTIES FOLDER Contrib)
set_target_properties(fmt PROPERTIES FOLDER Contrib)
set_target_properties(delegate PROPERTIES FOLDER Contrib)
set_target_properties(mysql PROPERTIES FOLDER Contrib)

