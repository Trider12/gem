package com.trider.gem;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
                "app"
        };
    }
}