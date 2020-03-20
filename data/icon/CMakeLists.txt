foreach(size 16 22 24 32 48 128)
    install(DIRECTORY ${size}x${size} DESTINATION share/icons/hicolor
             PATTERN .* EXCLUDE
             PATTERN *~ EXCLUDE)
endforeach(size 16 22 24 32 48 128)

install(DIRECTORY scalable DESTINATION share/icons/hicolor
         PATTERN ".*" EXCLUDE
         PATTERN "*~" EXCLUDE)
