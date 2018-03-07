-- Dump bluetooth SCO payload to raw pcm file (*.pcm)  
------------------------------------------------------------------------------------------------  
do  
    --local bit = require("bit") -- only work before 1.10.1  
    --local bit = require("bit32") -- only work after 1.10.1 (only support in Lua 5.2)  
    local version_str = string.match(_VERSION, "%d+[.]%d*")  
    local version_num = version_str and tonumber(version_str) or 5.1  
    local bit = (version_num >= 5.2) and require("bit32") or require("bit")  
  
    -- for geting h264 data (the field's value is type of ByteArray)  
    local f_h264 = Field.new("h264")   
    local f_rtp = Field.new("rtp")   
    local f_rtp_seq = Field.new("rtp.seq")  
    local f_rtp_timestamp = Field.new("rtp.timestamp")  
      
    --local function get_enum_name(list, index)  
    --    local value = list[index]  
    --    return value and value or "Unknown"  
    --end  
  
    -- menu action. When you click "Tools->Export H264 to file [HQX's plugins]" will run this function  
    local function export_sco_to_file()  
        -- window for showing information  
        local tw = TextWindow.new("Export sco to File Info Win")  
        --local pgtw = ProgDlg.new("Export sco to File Process", "Dumping sco data to file...")  
        local pgtw;  
          
        -- add message to information window  
        function twappend(str)  
            tw:append(str)  
            tw:append("\n")  
        end  
          
        -- trigered by all h264 packats  
        local my_h264_tap = Listener.new(tap, "hci_h4") 
       
    
        local function remove()  
            twappend("ready to close!")  
            --my_h264_tap:remove()  
        end      

        tw:set_atclose(remove)  
          
        local function export_sco(dir)  
            pgtw = ProgDlg.new("Export sco to File Process", "Dumping sco data to file...")  
            
            -- close progress window  
            pgtw:close()  
        end  
          
        local function export_send_packet()  
            export_sco(false)  
        end  
          
        local function export_recv_packet()  
            export_sco(true)  
        end  
          
        tw:add_button("Export local to remote", export_send_packet)  
        tw:add_button("Export remote to local", export_recv_packet)  
    end  
      
    -- Find this feature in menu "Tools->"Export H264 to file [HQX's plugins]""  
    register_menu("Export sco to file [Terry's plugins]", export_sco_to_file, MENU_TOOLS_UNSORTED)  
end  
