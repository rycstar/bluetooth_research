-- Dump bluetooth SCO payload to raw pcm file (*.pcm)  
------------------------------------------------------------------------------------------------  
do  
    --local bit = require("bit") -- only work before 1.10.1  
    --local bit = require("bit32") -- only work after 1.10.1 (only support in Lua 5.2)  
    local version_str = string.match(_VERSION, "%d+[.]%d*")  
    local version_num = version_str and tonumber(version_str) or 5.1  
    local bit = (version_num >= 5.2) and require("bit32") or require("bit")  
  
    -- for geting sco data (the field's value is type of ByteArray)  
    local f_hci_h4 = Field.new("hci_h4")
	local f_hci_h4_dir = Field.new("hci_h4.direction")
	local f_hci_h4_type = Field.new("hci_h4.type")
	local f_bthci_sco = Field.new("bthci_sco")
	local f_bthci_sco_len = Field.new("bthci_sco.length")
	local f_bthci_sco_data = Field.new("bthci_sco.data")
  
    -- menu action. When you click "Tools->Export sco to file [Terry's plugins]" will run this function  
    local function export_sco_to_file()  
        -- window for showing information  
        local tw = TextWindow.new("Export sco to File Info Win")  
        --local pgtw = ProgDlg.new("Export sco to File Process", "Dumping sco data to file...")  
        local pgtw;  
        local g_filename;  
        -- add message to information window  
        function twappend(str)  
            tw:append(str)  
            tw:append("\n")  
        end  
          
		local g_dir = 0  
		-- variable for storing rtp stream and dumping parameters  
        local stream_infos = nil   
        -- trigered by all h264 packats  
        local my_sco_tap = Listener.new(tap, "hci_h4") 
       
		-- call this function if a packet contains h264 payload  
        function my_sco_tap.packet(pinfo,tvb)
			if stream_infos == nil then  
                -- not triggered by button event, so do nothing.  
                return  
            end
			local hci_type =  f_hci_h4_type()
			local hci_dir =  f_hci_h4_dir()
			--twappend("type=" .. tostring(hci_type))
			--twappend("dir=" .. tostring(hci_dir))
			if hci_type.value == 0x03 then
				--twappend("a sco packet" .. tostring(hci_type))
				if hci_dir.value == g_dir then
					--local hci_data =  f_bthci_sco_data()
					--twappend("should write to file:"..tostring(hci_type).."dir : "..tostring(hci_h4.value:tvb()))
					local hci_h4 = f_bthci_sco()
					local sco = (version_num >= 5.2) and hci_h4.range:bytes() or hci_h4.value 
					local sco_len = f_bthci_sco_len()
					--if sco_len.value == 0x30 then
						--twappend("should write to size:"..tostring(sco_len))
						--g_filename:write((version_num >= 5.2) and sco:tvb():raw(3,sco_len.value) or sco:tvb()(3,sco_len.value):string())
					--end
					g_filename:write((version_num >= 5.2) and sco:tvb():raw(3,sco_len.value) or sco:tvb()(3,sco_len.value):string())
				end
			end
		end
	
        local function remove()  
            my_sco_tap:remove()  
        end      

        tw:set_atclose(remove)  
          
        local function export_sco(direction)  
            pgtw = ProgDlg.new("Export sco to File Process", "Dumping sco data to file...")  
			if direction == 1 then
				g_filename = io.open("sco_recv.pcm", "wb")
			else
				g_filename = io.open("sco_send.pcm", "wb")
			end
            stream_infos = {}
			g_dir = direction	
			retap_packets()
            -- close progress window  
			g_filename:flush()
			g_filename:close()
            pgtw:close()  
			stream_infos = nil
			g_filename = nil
        end  
          
        local function export_send_packet()  
            export_sco(0)  
        end  
          
        local function export_recv_packet()  
            export_sco(1)  
        end  
          
        tw:add_button("Export local to remote", export_send_packet)  
        tw:add_button("Export remote to local", export_recv_packet)  
    end  
      
    -- Find this feature in menu "Tools->"Export H264 to file [HQX's plugins]""  
    register_menu("Export sco to file [Terry's plugins]", export_sco_to_file, MENU_TOOLS_UNSORTED)  
end  
