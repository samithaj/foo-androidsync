// This file is part of foo_androidsync.

// foo_androidsync is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// foo_androidsync is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
   
// You should have received a copy of the GNU Lesser General Public License
// along with foo_androidsync.  If not, see <http://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "resource.h"
#include <playlist.h>

// = Declarations =

const pfc::string8 APP_NAME = "Android Sync";

void androidsync_basename( pfc::string8 &, pfc::string8 & );
void androidsync_remote( pfc::string8 &, pfc::string8 & );
void androidsync_do_sync_pl_items( 
   t_size, pfc::list_t<pfc::string8> &, pfc::list_t<pfc::string8> &
);
void androidsync_do_sync_pl( 
   t_size,
   pfc::list_t<pfc::string8> &,
   pfc::list_t<pfc::string8> &
);
void androidsync_do_sync_remove( 
   pfc::list_t<pfc::string8> &, pfc::list_t<pfc::string8> &
);
void androidsync_do_sync();

DECLARE_COMPONENT_VERSION(
   "Android Sync Component",
   "0.8.10",
   "Select the playlists you would like to synchronize from the \"Synchronize "
   "Playlists\" preference panel and then select \"Android Sync\" -> \""
   "Synchronize Playlists\" from the file menu to perform synchronization."
);

// This will prevent users from renaming your component around (important for 
// proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME( "foo_androidsync.dll" );

static const GUID g_mainmenu_group_id = {
   // {ACE8A3BE-8E51-4FBA-BB22-4560643BC346}
   0xace8a3be, 0x8e51, 0x4fba, 
   { 0xbb, 0x22, 0x45, 0x60, 0x64, 0x3b, 0xc3, 0x46 }
};

// These GUIDs identify the variables within our component's configuration file.
static const GUID guid_cfg_selectedplaylists = {
   // {C1D68A14-464C-4B44-8DDE-A1723AFD881F}
   0xc1d68a14, 0x464c, 0x4b44, 
   { 0x8d, 0xde, 0xa1, 0x72, 0x3a, 0xfd, 0x88, 0x1f } 
};

static const GUID guid_cfg_targetpath = {
   // {A0EF91CA-E192-4F5A-9DA3-0B5F300E26C8}
   0xa0ef91ca, 0xe192, 0x4f5a, { 0x9d, 0xa3, 0xb, 0x5f, 0x30, 0xe, 0x26, 0xc8 }
};

static cfg_objList<pfc::string8> cfg_selectedplaylists( guid_cfg_selectedplaylists );
static cfg_string cfg_targetpath( guid_cfg_targetpath, "" );



/* = Main Menu Additions = */

static mainmenu_group_popup_factory g_mainmenu_group( 
   g_mainmenu_group_id, 
   mainmenu_groups::file, 
   mainmenu_commands::sort_priority_dontcare, 
   "Android Sync"
);

class mainmenu_commands_androidsync : public mainmenu_commands {
public:
	enum {
		cmd_sync = 0,
		cmd_total
	};

	t_uint32 get_command_count() {
		return cmd_total;
	}

	GUID get_command( t_uint32 p_index ) {
		static const GUID guid_sync = {
         // {F24A7301-5B86-420B-A1E8-B1EE83CEF4DD}
         0xf24a7301, 0x5b86, 0x420b, 
         { 0xa1, 0xe8, 0xb1, 0xee, 0x83, 0xce, 0xf4, 0xdd }

      };

		switch(p_index) {
			case cmd_sync: 
            return guid_sync;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	void get_name( t_uint32 p_index, pfc::string_base &p_out ) {
		switch( p_index ) {
			case cmd_sync: 
            p_out = "Synchronize Playlists";
            break;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	bool get_description( t_uint32 p_index,pfc::string_base &p_out ) {
		switch( p_index ) {
			case cmd_sync:
            p_out = "Synchronize the playlists selected in preferences to "
               "the connected android device";
            return true;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	GUID get_parent() {
		return g_mainmenu_group_id;
	}

	void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) {
		switch( p_index ) {
			case cmd_sync:
				androidsync_do_sync();
				break;

			default:
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
				uBugCheck();
		}
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_androidsync> 
   g_mainmenu_commands_androidsync_factory;



/* = Preferences = */

class CAndroidSyncPrefs : public CDialogImpl<CAndroidSyncPrefs>, 
public preferences_page_instance {
public:
	// Constructor, invoked by preferences_page_impl helpers. Don't do Create()
   // in here, preferences_page_impl does this for us.
	CAndroidSyncPrefs( preferences_page_callback::ptr callback ) : 
      m_callback( callback ) {}

	// Note that we don't bother doing anything regarding destruction of our 
   // class. The host ensures that our dialog is destroyed first, then the 
   // last reference to our preferences_page_instance object is released, 
   // causing our object to be deleted.

	// Dialog Resource ID
	enum { IDD = IDD_ANDROIDSYNCPREFS };

   // preferences_page_instance methods (not all of them - get_wnd() is 
   // supplied by preferences_page_impl helpers).
	t_uint32 get_state() {
	   t_uint32 state = preferences_state::resettable;
	
      if( has_changed() ) {
         state |= preferences_state::changed;
      }

	   return state;
   }

   void apply() {
	   // cfg_selectedplaylists = GetDlgItemInt(IDC_SELECTEDPLAYLISTS, NULL, FALSE);
	   // cfg_targetpath = GetDlgItemInt(IDC_TARGETPATH, NULL, FALSE);
      int item_count,
         i,
         item_index_iter,
         * item_indexes;
      pfc::string8 item_iter_8;
      LPTSTR targetpath_wide = new TCHAR[MAX_PATH];

      // Clear the existing list of selected playlists before we begin.
      cfg_selectedplaylists.remove_all();

      // Get the currently selected playlists from the listbox.
      item_count = SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELCOUNT,
         NULL, NULL
      );
      item_indexes = new int[item_count];
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELITEMS,
         (WPARAM)item_count,
         (LPARAM)item_indexes
      );

      // Save the currently selected list of playlists to the configuration
      // variable.
      for( i = 0 ; i < item_count ; i++ ) {
         item_index_iter = item_indexes[i];
      
         // XXX: This just seems to return blank strings/crashes/heartache.
         /* item_iter_wide = (LPCTSTR)SendDlgItemMessage(
            IDC_SELECTEDPLAYLISTS,
            LB_GETITEMDATA,
            (WPARAM)item_index_iter,K
            0
         ); */
      
         // This is kind of cheating since we're not pulling it from the listbox
         // itself, but it will probably work for the time being since we're 
         // pulling from the same source it gets the names from.
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            item_index_iter, item_iter_8 
         );

         cfg_selectedplaylists.add_item( item_iter_8 );
      }
      delete item_indexes;

      // Save the current target path.
      GetDlgItemText( IDC_TARGETPATH, targetpath_wide, MAX_PATH );
      cfg_targetpath.set_string(
         pfc::stringcvt::string_utf8_from_wide( targetpath_wide )
      );

      // Our dialog content has not changed but the flags have - our currently 
      // shown values now match the settings so the apply button can be disabled.
	   on_change();
   }

   void reset() {
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_SETSEL,
         (WPARAM)FALSE,
         (LPARAM)-1
      );
      SetDlgItemText( IDC_TARGETPATH, _T( "" ) );

      on_change();
   }

	// WTL Message Map
	BEGIN_MSG_MAP( CAndroidSyncPrefs )
		MSG_WM_INITDIALOG( on_init_dialog )
		COMMAND_HANDLER_EX( IDC_SELECTEDPLAYLISTS, LBN_SELCHANGE, on_sel_change )
		COMMAND_HANDLER_EX( IDC_TARGETPATH, EN_CHANGE, on_edit_change )
	END_MSG_MAP()

private:

   BOOL on_init_dialog(CWindow, LPARAM) {
      t_size plist_id_iter, // ID of the currently iterated playlist.
         cpare_id_iter; // ID of the string plist_iter is compared against.
      pfc::string8 cpare_iter,
         playlist_name;

      // Populate the list box with all available playlists.
      for( 
         plist_id_iter = 0;
         plist_id_iter < static_api_ptr_t<playlist_manager>()->get_playlist_count();
         plist_id_iter++
      ) {
   
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            plist_id_iter, playlist_name 
         );

         // Modern Foobar plugin projects have to be unicode-enabled, which causes
         // all Windows API functions to require wide character inputs. However,
         // the playlist manager function above uses the proprietary PFC string 
         // class which is not wide. Therefore, the string must be converted using 
         // the stringcvt helper function. This information was found conveniently 
         // in the cellar of the Foobar documentation, with the aid of a 
         // flashlight, in the bottom of a locked filing cabinet stuck in a disused
         // lavatory with a sign on the door saying, "Beware of the Leopard".
         SendDlgItemMessage(
            IDC_SELECTEDPLAYLISTS,
            LB_ADDSTRING,
            (WPARAM)0,
            (LPARAM)((LPCTSTR)pfc::stringcvt::string_wide_from_utf8( playlist_name ))
         );

         // Select user-selected playlists in the list box.
         for( 
            cpare_id_iter = 0;
            cpare_id_iter < cfg_selectedplaylists.get_count();
            cpare_id_iter++
         ) {
            cpare_iter = cfg_selectedplaylists.get_item( cpare_id_iter );

            if( 0 == pfc::comparator_strcmp::compare( cpare_iter, playlist_name ) ) {
               SendDlgItemMessage(
                  IDC_SELECTEDPLAYLISTS,
                  LB_SETSEL,
                  (WPARAM)TRUE,
                  (LPARAM)plist_id_iter
               );
            }
         }
      }

      // Set the target path control to the user-selected value.
      SetDlgItemText(
         IDC_TARGETPATH, 
         pfc::stringcvt::string_wide_from_utf8( cfg_targetpath )
      );
   
	   return FALSE;
   }

   void on_edit_change(UINT, int, CWindow) {
	   on_change();
   }

   void on_sel_change(UINT, int, CWindow) {
      on_change();
   }

   void on_change() {
	   // Tell the host that our state has changed to enable/disable the apply 
      // button appropriately.
	   m_callback->on_state_changed();
   }

   // Returns whether our dialog content is different from the current 
   // configuration (whether the apply button should be enabled or not).
   bool has_changed() {
	   /* LPCTSTR targetpath_text;
      bool changed = false;
      int item_count,
         i,
         item_index_iter,
         * item_indexes;
      pfc::string8 item_iter_8;

      // Get the currently selected playlists from the listbox.
      item_count = SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELCOUNT,
         NULL, NULL
      );
      item_indexes = new int[item_count];
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELITEMS,
         (WPARAM)item_count,
         (LPARAM)item_indexes
      ); */

      // See if the playlist selections have changed.
      /* for( i = 0 ; i < item_count ; i++ ) {
         item_index_iter = item_indexes[i];

         // This is kind of cheating since we're not pulling it from the listbox
         // itself, but it will probably work for the time being since we're 
         // pulling from the same source it gets the names from.
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            item_index_iter, item_iter_8 
         );

         if( 0 != cfg_selectedplaylists.find_item( item_iter_8 ) ) {
         
         }
      }
      delete item_indexes; */

      // See if the target path changed.
      /* GetDlgItemText( IDC_SELECTEDPLAYLISTS, targetpath_text,  );
      if( 0 != pfc::comparator_strcmp::compare( targetpath_text, cfg_targetpath ) ) {
         changed = true;
      } */

      // return changed;
      // XXX
      return true;
   }
   
   const preferences_page_callback::ptr m_callback;
};

class preferences_page_androidsync : 
public preferences_page_impl<CAndroidSyncPrefs> {
public:
   const char * get_name() { return APP_NAME; }

	GUID get_guid() {
		static const GUID guid = {
         // {77329F7B-F28A-404A-8A0B-5F50426F4211}
         0x77329f7b, 0xf28a, 0x404a, 
         { 0x8a, 0xb, 0x5f, 0x50, 0x42, 0x6f, 0x42, 0x11 }
      };

      return guid;
	}

	GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_androidsync>
   g_preferences_page_androidsync_factory;



/* = Synchronization = */

// Figure out the base name of the given item. 
void androidsync_basename( pfc::string8 &path_in, pfc::string8 &basename_out ) {
   const char* item_basename_start;

   // Setup.
   basename_out.reset();

   // Base name is the file name minus the path. Maybe there's a better way to 
   // do this?
   item_basename_start = strrchr( path_in, '\\' );
   basename_out << 
      (item_basename_start ? item_basename_start + 1 : path_in);
}

// Figure out the destination/target path for the given item. 
void androidsync_remote( pfc::string8 &path_in, pfc::string8 &remote_out ) {
   pfc::string8 item_basename;
   
   // Setup.
   androidsync_basename( path_in, item_basename );
   remote_out.reset();

   // Remote name is the target path plus the base name.
   remote_out << cfg_targetpath;
   if( !cfg_targetpath.ends_with( '\\' ) ) {
      // There's no trailing backslash, so add one.
      remote_out.add_char( '\\' );
   }
   remote_out << item_basename;
}

// Copy all of the files on a given playlist to the target directory as 
// specified in the configuration options.
void androidsync_do_sync_pl_items( 
   t_size playlist_id_in, 
   pfc::list_t<pfc::string8> &all_playlist_items,
   pfc::list_t<pfc::string8> &copied_playlist_items
) {
   pfc::list_t<metadb_handle_ptr> playlist_items; // List of plist items.
   pfc::string8 item_iter,
      item_iter_remote, // Remote path to post-copy item.
      item_iter_basename;
   t_size item_iter_id; // Index of the currently processing plist item.
   SHFILEOPSTRUCT op;
   int i, // General purpose iterator.
      copy_result = 0,
      src_len = 0,
      dst_len = cfg_targetpath.get_length() + 2; // +2 for double NULL.
   TCHAR* t_src,
      * t_dst;
   pfc::string_formatter src8;

   // Build the source path list.
   static_api_ptr_t<playlist_manager>()->playlist_get_all_items(
      playlist_id_in, playlist_items
   );
   for( 
      item_iter_id = 0;
      item_iter_id < playlist_items.get_count();
      item_iter_id++
   ) {
      filesystem::g_get_display_path( 
         playlist_items.get_item( item_iter_id ).get_ptr()->get_path(), 
         item_iter 
      );

      // Figure out the base name and remote name of the current item. 
      androidsync_basename( item_iter, item_iter_basename );
      androidsync_remote( item_iter, item_iter_remote );

      // Add this file to the list of all items.
      all_playlist_items.add_item( item_iter_basename );

      // Only add the file to the list if it doesn't already exist at the 
      // destination.
      // TODO: Compare source and destination based on size or modification 
      //       times?
      if( 0xFFFFFFFF == GetFileAttributes( 
         pfc::stringcvt::string_wide_from_utf8( item_iter_remote )
      ) ) {
         src8 << item_iter;
         src8.add_char( '|' );
         src_len += item_iter.get_length() + 1; // +1 for the NULL.

         // Make a note that we were able to copy this item.
         copied_playlist_items.add_item( item_iter_basename );
      }
   }
   
   // Convert the source and destination paths into char arrays.
   t_src = new TCHAR[src_len + 1]; // +1 for the second NULL.
   t_dst = new TCHAR[dst_len];
   memset( t_src, NULL, (src_len + 1) * sizeof( TCHAR ) );
   memset( t_dst, NULL, dst_len * sizeof( TCHAR ) );
   pfc::stringcvt::convert_utf8_to_wide( t_src, src_len, src8, src_len );
   pfc::stringcvt::convert_utf8_to_wide(
      t_dst, dst_len - 1, cfg_targetpath, cfg_targetpath.length()
   );

   // Convert the '|' deliminators to NULLs.
   for( i = 0 ; i < src_len ; i++ ) {
      if( _T( '|' ) == t_src[i] ) {
         t_src[i] = _T( '\0' );
      }
   }
   
   // Build the file copy instruction structure.
   memset( &op, NULL, sizeof( op ) );
	op.wFunc = FO_COPY;
   op.pFrom = t_src;
   op.pTo = t_dst;
	op.fFlags = FOF_NOCONFIRMMKDIR;
   op.hwnd = core_api::get_main_window();

   // Perform the actual copy.
   copy_result = SHFileOperation( &op );
   if( copy_result ) {
      popup_message::g_show( "Copy to device failed.", APP_NAME );
   }

   // Cleanup
   delete t_src;
   delete t_dst;
}

// Write the specified playlist to the target directory set in the configuration
// options, modified to point to files in the same directory.
void androidsync_do_sync_pl( 
   t_size playlist_id_in, 
   pfc::list_t<pfc::string8> &all_playlist_items,
   pfc::list_t<pfc::string8> &copied_playlist_items   
) {
   pfc::list_t<metadb_handle_ptr> playlist_items; // List of plist items.
   pfc::string8 item_iter,
      playlist_name,
      playlist_path_remote,
      item_iter_remote, // Remote path to post-copy item.
      item_iter_basename;
   t_size item_iter_id; // Index of the currently processing plist item.
   FILE* playlist;

   // Figure out the remote name to write the playlist to.
   static_api_ptr_t<playlist_manager>()->playlist_get_name(
      playlist_id_in, playlist_name
   );
   playlist_name << ".m3u";
   androidsync_remote( playlist_name, playlist_path_remote );

   // The playlist is a copied file too!
   all_playlist_items.add_item( playlist_name );
   copied_playlist_items.add_item( playlist_name );

   // Open the file.
   fopen_s( &playlist, playlist_path_remote, "w" );
   if( NULL == playlist ) {
      // Problem!
      popup_message::g_show(
         pfc::string8() << "Error opening playlist " << playlist_name << 
            " for writing on device.",
         APP_NAME
      );
      return;
   }
   
   // Build the source path list.
   static_api_ptr_t<playlist_manager>()->playlist_get_all_items(
      playlist_id_in, playlist_items
   );
   for( 
      item_iter_id = 0;
      item_iter_id < playlist_items.get_count();
      item_iter_id++
   ) {
      filesystem::g_get_display_path( 
         playlist_items.get_item( item_iter_id ).get_ptr()->get_path(), 
         item_iter
      );

      // Figure out the base name and remote name of the current item. 
      androidsync_basename( item_iter, item_iter_basename );

      // Write the line for the current item in the output playlist.
      fwrite(
         item_iter_basename,
         sizeof( char ),
         item_iter_basename.get_length(),
         playlist
      );
      fwrite( "\n", sizeof( char ), 1, playlist );
   }

   // Cleanup.
   fclose( playlist );
}

// Check the files in the target directory and remove any that aren't playlists 
// or music files.
void androidsync_do_sync_remove( 
   pfc::list_t<pfc::string8> &all_playlist_items,
   pfc::list_t<pfc::string8> &removed_items 
) {
   WIN32_FIND_DATA find_data;
   HANDLE find_handle = INVALID_HANDLE_VALUE;
   DWORD result_error = 0;
   pfc::string8 search_target,
      delete_file_path,
      delete_file_name;
   t_size playlist_idx_iter;
   bool item_found;

   // Clean up a string with the target path.
   search_target << cfg_targetpath;
   if( !search_target.ends_with( '\\' ) ) {
      // FindFirstFile and co. don't like trailing backslashes.
      search_target.add_chars( '\\', 1 );
   }
   search_target << "*.mp3";

   // Find the first file in the directory.
   find_handle = FindFirstFile( pfc::stringcvt::string_wide_from_utf8( search_target ), &find_data );
   if( INVALID_HANDLE_VALUE == find_handle ) {
      popup_message::g_show(
         pfc::string8() << "There was a problem examining the directory for removal.",
         APP_NAME
      );
   } 
   
   do {
      // Check the file against the list of all copied files and delete it if 
      // it's not present.
      // TODO: Is there a more effective/quicker way of doing this? We're not 
      //       very good with algorithms.
      item_found = false;
      for( 
         playlist_idx_iter = 0;
         playlist_idx_iter < all_playlist_items.get_count();
         playlist_idx_iter++
      ) {
         if( 0 == strcmp( 
            pfc::stringcvt::string_utf8_from_wide( find_data.cFileName ),
            all_playlist_items.get_item( playlist_idx_iter ) 
         ) ) {
            // The item from the file system is present in the list, so move 
            // on to the next step.
            item_found = true;
            break;
         }
      }

      if( !item_found ) {
         // The filename could not be found in the list of playlist files, so
         // delete the file.
         delete_file_name.reset();
         delete_file_name <<
            pfc::stringcvt::string_utf8_from_wide( find_data.cFileName );
         androidsync_remote( delete_file_name, delete_file_path );
         DeleteFile( pfc::stringcvt::string_wide_from_utf8( delete_file_path ) );
         removed_items.add_item( pfc::stringcvt::string_utf8_from_wide( find_data.cFileName ) );
      }     
   } while( 0 != FindNextFile( find_handle, &find_data ) );

   FindClose( find_handle );
}

void androidsync_do_sync() {
   t_size plist_id_iter, // Iterator for all playlist IDs.
       selected_id_iter; // Iterator for selected playlist IDs.
   pfc::string8 plist_iter,
      selected_iter;
   pfc::list_t<pfc::string8> all_playlist_items,
      copied_playlist_items,
      removed_items;

   // Make sure the target path is available. Quit if it's not.
   if( 0xFFFFFFFF == GetFileAttributes( 
      pfc::stringcvt::string_wide_from_utf8( cfg_targetpath )
   ) ) {
      popup_message::g_show(
         pfc::string8() << "The target path specified, \"" << cfg_targetpath <<
            "\", is not available. Please mount your Android device or fix " <<
            "the path specified in the " << APP_NAME << " options.",
         APP_NAME
      );
      return;
   }

   for( 
      plist_id_iter = 0;
      plist_id_iter < static_api_ptr_t<playlist_manager>()->get_playlist_count();
      plist_id_iter++
   ) {
      for(
         selected_id_iter = 0;
         selected_id_iter < cfg_selectedplaylists.get_count();
         selected_id_iter++
      ) {
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            plist_id_iter, plist_iter
         );
         selected_iter = cfg_selectedplaylists.get_item( selected_id_iter );
         if( 
            0 == pfc::comparator_strcmp::compare( plist_iter, selected_iter )
         ) {
            // This playlist is on the list of selected playlists.
            androidsync_do_sync_pl(
               plist_id_iter,
               all_playlist_items,
               copied_playlist_items 
            );
            androidsync_do_sync_pl_items( 
               plist_id_iter,
               all_playlist_items,
               copied_playlist_items
            );
            break;
         }
      }
   }

   // Remove files in the target directory which aren't music or playlist 
   // files.
   androidsync_do_sync_remove( all_playlist_items, removed_items );

   // Report results.
   popup_message::g_show(
      pfc::string8() << all_playlist_items.get_count() << " items checked.\n" <<
         copied_playlist_items.get_count() << " items written.\n" <<
         (all_playlist_items.get_count() - copied_playlist_items.get_count()) << 
         " items skipped.\n" << removed_items.get_count() << " items removed.", 
      APP_NAME
   );
}
