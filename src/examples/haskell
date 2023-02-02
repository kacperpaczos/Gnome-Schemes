{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE OverloadedLabels  #-}

import qualified GI.Gio as Gio
import qualified GI.Gtk as Gtk
import Control.Monad (void)
import Data.GI.Base
import qualified Data.Text as T (pack)
import System.Environment (getArgs)

main :: IO ()
main = do
  app <- Gtk.applicationNew (Just "com.example.hello") []

  on app #activate (onActivate app)

  args <- getArgs
  void $ #run app (Just $ T.pack <$> args)

  where
    onActivate :: Gtk.Application -> IO ()
    onActivate app = do
      win <- new Gtk.Window [ #type := Gtk.WindowTypeToplevel
                            , #defaultWidth := 600
                            , #defaultHeight := 300 ]
      #addWindow app win
      
      headerBar <- new Gtk.HeaderBar [ #title :=  "Hello, World!"
                                     , #showCloseButton := True ]
      #show headerBar
      #setTitlebar win (Just headerBar)

      label <- new Gtk.Label [ #label := "<span weight=\"bold\" size=\"larger\">Hello, World!</span>"
                             , #useMarkup := True ]
      #show label
      #add win label

      #present win
