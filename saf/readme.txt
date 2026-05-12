- Symta archive file layout (SAF):
  - Files (data for each file in sequence)
  - File table, each entry of which contains:
    - Uncompressed size
    - File offset (compressed stream size is calculated from offsets)
    - Flags (folder/file, compression method)
    - Filename (zero-terminated)
  - Header (4 32-bit fields)
    - Magic "\1SAF"
    - File table offset
    - Flags
    - Number of files

- Note1: Filetable gets converted to a file-path keyed hashtable on load.
  That allows quickly resolving paths. Alternative format version
  could use hashes instead of filenames to speed up access
  and obfuscate the internal structure.
  Yet many applications require listing folder content.
- Note2: a folder name refereces a list of all files inside.
  as a \n-separate list. That way such strings can be quickly used without
  additional transformations.
  It is required to quickly traverse folders.
- Note3: all files inside the archive have the creation time of the archive
