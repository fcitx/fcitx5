foreach(size 16 22 24 32 48 128)
    install(DIRECTORY ${size}x${size} DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor"
             PATTERN .* EXCLUDE
             PATTERN *~ EXCLUDE)
endforeach(size 16 22 24 32 48 128)

install(DIRECTORY scalable DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor"
         PATTERN ".*" EXCLUDE
         PATTERN "*~" EXCLUDE)
