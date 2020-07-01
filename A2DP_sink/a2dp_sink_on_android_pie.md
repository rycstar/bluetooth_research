**Bluetooth Sound box on Android Pie**

  1. preview
  
       A BT sound box is a audio player which can receive the Song from another BT device.
       
       Generally, we can play the music from SmartPhone, TV and etc.
       
       Further more, if the sound box has a display panel, we can display the metadata of Song, and sent control msg to request next/prev/pause/play on target device.
  
  2. profile selected on Android Pie
    
      For sound box, we need to enable two profiles:
      
      a) A2DP sink Profile (mandatory)
      
          A2DP sink profile mainly to receive audio from SmartPhone.
          Also, the SmartPhone need to enable A2dp Profile to connect sound box.
      
      b) AVRCP controller Profile (optional)
       
         If sound box don't support Song Information display or controller functions, this profile can be disabled.
         
         AVRCP controller Profile provide a protocol to get/set Song status(Information,like Lyric) on SmartPhone.
         
      c) How to enable profile on Android?
      
         Edit config.xml in /packetages/app/Bluetooth/res/value (If there is a overlay path, edit it in overlay path)
         
         For example: (disable A2DP,enable A2DP sink, disable AVRCP target, enable AVRCP controller)
         
             <bool name="profile_supported_a2dp">false</bool>
             <bool name="profile_supported_a2dp_sink">true</bool>
             <bool name="profile_supported_avrcp_target">false</bool>
             <bool name="profile_supported_avrcp_controller">true</bool>
             
         Notice: android bluedroid stack can't support A2DP and A2DP sink in the same time, also can't support avrcp_target and avrcp_controller in the same time.
  3. Arch analysis
  
      In this document, we'll focus on frameworks layer to show how android support sound box.
      
      a)  BluetoothA2dpSink.java  provided the method for App to connect Phone, at last it'll call function in Bluedroid stack to connect.
      
      b)  If devices connected or status changed, Bluedroid stack will call JNI method to notify Bluetooth service 
      
      c)  Sony player is done in system/bt which is not included in bluetooth.apk, bluedroid stack created a audiotrack and play the decoded data.
      you can find it in file btif_a2dp_sink.cc and btif_avrcp_audio_track.cc
      
      ------------------------------------------------framework layer API for applications-------------------------------------------------                    
                         BluetoothA2dpSink.java                                      BluetoothAvrcpController.java   
      -------------------------------------------------------------------------------------------------------------------------------------
          --------------------------------------------Bluetooth service in bluetooth.apk--------------------------------------------
          |               A2dpSinkService.java                    |                   A2dpMediaBrowserSevice.java                   |
          |                                                       |                                                                 |
          |               A2dpSinkStreamHandler.java              |                   AvrcpControllerStateMachine.java              |
          |                                                       |                                                                 |
          |               A2dpSinkStateMachine.java               |                   AvrcpControllerService.java                   |
          --------------------------------------------------------------------------------------------------------------------------
                                      ^                                                              ^
                                      |                                                              |
                                      |                                                              |
                                      v                                                              v
           ------------------------------------------------------------Native API-----------------------------------------------------
           |        com_android_bluetooth_a2dp_sink.cpp                   |          com_android_bluetooth_avrcp_controller.cpp       |
           |                                                              |                                                           |
           |--------------------------------------------------------------------------------------------------------------------------
           
           ----------------------------------------------------Bluedroid stack---------------------------------------------------------
  4. App develop guide
     
     As said above, we can see that, we don't need to handle any audio data as btif in bluedroid stack has done for us.
     So for Application, mainly is to develop a beautiful app to control Song player and display Song information
     
     Many articles describe that we need to call APIs  in BluetoothAvrcpController.java to send GroupNavigationCmd/passthroughCmd etc.
     But actually, Android has done it for us in A2dpMediaBrowserSevice.java, it hide the detail information in AVRCP protocol, you don't need to
     understand what's passthroughCmd and GroupNavigationCmd, just think about this is a local music player.
     
     A2dpMediaBrowserSevice.java is a extend MediaBrowserService class, Application can use MediaSession framework to communicate with it.
     
     
 5. Reference
 
     1. http://read.pudn.com/downloads780/doc/3087534/AVRCP_SPEC_V15.pdf
     2. https://blog.csdn.net/weixin_42229694/article/details/89315026
