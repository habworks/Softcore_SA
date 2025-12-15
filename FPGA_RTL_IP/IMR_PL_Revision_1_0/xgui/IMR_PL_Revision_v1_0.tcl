# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "C_S00_AXI_ADDR_WIDTH" -parent ${Page_0}
  ipgui::add_param $IPINST -name "C_S00_AXI_BASEADDR" -parent ${Page_0}
  ipgui::add_param $IPINST -name "C_S00_AXI_HIGHADDR" -parent ${Page_0}

  ipgui::add_param $IPINST -name "C_S00_AXI_DATA_WIDTH" -widget comboBox
  ipgui::add_param $IPINST -name "C_REVISION_MAJOR"
  ipgui::add_param $IPINST -name "C_REVISION_MINOR"
  ipgui::add_param $IPINST -name "C_REVISION_TEST"

}

proc update_PARAM_VALUE.C_REVISION_MAJOR { PARAM_VALUE.C_REVISION_MAJOR } {
	# Procedure called to update C_REVISION_MAJOR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_REVISION_MAJOR { PARAM_VALUE.C_REVISION_MAJOR } {
	# Procedure called to validate C_REVISION_MAJOR
	return true
}

proc update_PARAM_VALUE.C_REVISION_MINOR { PARAM_VALUE.C_REVISION_MINOR } {
	# Procedure called to update C_REVISION_MINOR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_REVISION_MINOR { PARAM_VALUE.C_REVISION_MINOR } {
	# Procedure called to validate C_REVISION_MINOR
	return true
}

proc update_PARAM_VALUE.C_REVISION_TEST { PARAM_VALUE.C_REVISION_TEST } {
	# Procedure called to update C_REVISION_TEST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_REVISION_TEST { PARAM_VALUE.C_REVISION_TEST } {
	# Procedure called to validate C_REVISION_TEST
	return true
}

proc update_PARAM_VALUE.C_S00_AXI_DATA_WIDTH { PARAM_VALUE.C_S00_AXI_DATA_WIDTH } {
	# Procedure called to update C_S00_AXI_DATA_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_S00_AXI_DATA_WIDTH { PARAM_VALUE.C_S00_AXI_DATA_WIDTH } {
	# Procedure called to validate C_S00_AXI_DATA_WIDTH
	return true
}

proc update_PARAM_VALUE.C_S00_AXI_ADDR_WIDTH { PARAM_VALUE.C_S00_AXI_ADDR_WIDTH } {
	# Procedure called to update C_S00_AXI_ADDR_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_S00_AXI_ADDR_WIDTH { PARAM_VALUE.C_S00_AXI_ADDR_WIDTH } {
	# Procedure called to validate C_S00_AXI_ADDR_WIDTH
	return true
}

proc update_PARAM_VALUE.C_S00_AXI_BASEADDR { PARAM_VALUE.C_S00_AXI_BASEADDR } {
	# Procedure called to update C_S00_AXI_BASEADDR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_S00_AXI_BASEADDR { PARAM_VALUE.C_S00_AXI_BASEADDR } {
	# Procedure called to validate C_S00_AXI_BASEADDR
	return true
}

proc update_PARAM_VALUE.C_S00_AXI_HIGHADDR { PARAM_VALUE.C_S00_AXI_HIGHADDR } {
	# Procedure called to update C_S00_AXI_HIGHADDR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_S00_AXI_HIGHADDR { PARAM_VALUE.C_S00_AXI_HIGHADDR } {
	# Procedure called to validate C_S00_AXI_HIGHADDR
	return true
}


proc update_MODELPARAM_VALUE.C_S00_AXI_DATA_WIDTH { MODELPARAM_VALUE.C_S00_AXI_DATA_WIDTH PARAM_VALUE.C_S00_AXI_DATA_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_S00_AXI_DATA_WIDTH}] ${MODELPARAM_VALUE.C_S00_AXI_DATA_WIDTH}
}

proc update_MODELPARAM_VALUE.C_S00_AXI_ADDR_WIDTH { MODELPARAM_VALUE.C_S00_AXI_ADDR_WIDTH PARAM_VALUE.C_S00_AXI_ADDR_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_S00_AXI_ADDR_WIDTH}] ${MODELPARAM_VALUE.C_S00_AXI_ADDR_WIDTH}
}

proc update_MODELPARAM_VALUE.C_REVISION_MAJOR { MODELPARAM_VALUE.C_REVISION_MAJOR PARAM_VALUE.C_REVISION_MAJOR } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_REVISION_MAJOR}] ${MODELPARAM_VALUE.C_REVISION_MAJOR}
}

proc update_MODELPARAM_VALUE.C_REVISION_MINOR { MODELPARAM_VALUE.C_REVISION_MINOR PARAM_VALUE.C_REVISION_MINOR } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_REVISION_MINOR}] ${MODELPARAM_VALUE.C_REVISION_MINOR}
}

proc update_MODELPARAM_VALUE.C_REVISION_TEST { MODELPARAM_VALUE.C_REVISION_TEST PARAM_VALUE.C_REVISION_TEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_REVISION_TEST}] ${MODELPARAM_VALUE.C_REVISION_TEST}
}

