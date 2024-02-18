#  impluse engine: Driving HFS volumes forward

<p style="float: right"><img width="128" height="128" alt="impluse's icon, showing a Mac OS 9 hard disk icon dissolving through a column of green plusses into a modern macOS volume icon." src="impluse icon.png" /></p>

Got an old-school HFS (Mac OS Standard) volume that you can't mount anymore since macOS dropped support for it?

impluse is an open-source tool that converts HFS volumes into HFS+ (Mac OS Extended) volumes that you can then mount.

impluse is:

- intentionally misspelled (it's “impulse” rearranged to have “plus” in it)
- still pronounced “impulse” though
- not an in-place converter: you must have a source and a destination (such as a read-only disk image, and an empty disk image large enough to hold its contents)
- a command-line tool, for now
- for modern macOS machines (Catalina and later)
- ⚠️ brand-new and untested ⚠️

impluse does not guarantee to:
- maintain locations of files within each volume
- change locations of files within each volume (e.g., don't expect it to defragment or prune free space)
- change allocation block size (HFS+ can, theoretically, have a smaller allocation block size for many/most volumes, which was one of its selling points over HFS)
- maintain the same allocation block size
- preserve catalog or extents-overflow entries for deleted files (if such entries exist in either file but are not reachable from the root file/folder node)
- prune catalog or extents-overflow entries for deleted files
- decode filenames correctly (there may be an option at some point to specify an encoding hint)
- extract working aliases in all cases (extracted aliases need to be reconnected to their destinations, which isn't always feasible)
- produce the optimal HFS+ volume for the input contents (e.g., DiskWarrior or PlusOptimizer may have a lot of improvements to suggest)
- produce an HFS wrapper volume for the HFS+ output

impluse's goals *do* include:
- reproduce file contents, including resource forks, accurately (should be testable by hashing any file on both volumes)
- reproduce file metadata, excluding filenames, accurately (e.g., creation/modification dates shouldn't change)
- produce a volume that mounts successfully on modern macOS, at least read-only
- pass HFS+ consistency checks (fsck, DiskWarrior, etc.)

## What impluse can do

There are two ways to use impluse.

One is as an “unzipper” that can treat an HFS volume like a zip archive. `impluse list` will produce the entire folder and file hierarchy from an HFS volume. `impluse extract` will extract items by name or path. If you extract a folder, all of its contents will be extracted as well. You should extract items to an HFS+ or APFS volume, because impluse will try to reproduce as much as it can from the original volume, including resource forks and Finder flags (so, for example, aliases will still be aliases).

The other is as a converter from HFS to HFS+. (If you remember Alsoft's old PlusMaker product, this is like that, but uglier.) `impluse convert` takes two pathnames, one containing the HFS volume to read from, and the other being a file or device to write the HFS+ volume to. *IMPORTANT:* If you pass a device (or existing file) to `impluse convert`, impluse will simply overwrite it without asking. Be very careful in checking for typos!

Because impluse is a new, sparsely-tested program, I highly recommend preserving a verbatim copy of your HFS volumes alongside any converted HFS+ images or extracted data. It would suck if you found a defect in the converted/extracted copy and had no original to redo the operation from.

## Getting an HFS volume where impluse can see it

### Reading from physical devices (CDs, HDDs, etc.)

If you have a physical device such as a hard disk or CD-ROM that is formatted as HFS, my recommendation (and in some cases a necessity) is to image the device first. I recommend imaging it as UDIF (.dmg), as that format will include checksums to detect corruption of the original image. This image becomes your verbatim original for preservation (i.e., don't delete the image when you're done) and any future extractions or conversions.

First you will need to attach the device.

- For a hard disk drive: Plug your HDD into a USB port. macOS will present a prompt asking whether you want to initialize, ignore, or eject the volume. **Choose Ignore.** (Note that “Initialize” means “erase everything”!)
- For a CD-ROM: Insert the CD. macOS will present a prompt asking whether you want to ignore or eject the volume. **Choose Ignore.**

In either case, nothing further will happen. The volume won't mount on the desktop, since macOS doesn't support HFS, and (if it's a CD or other removable disk) it won't be ejected.

Now that it's attached, you can image it. You can do this in Disk Utility, though I recommend using `hdiutil` instead, as it gives you more precise control over what will be imaged and how.

Use `diskutil list` to find which device is the one you've just connected. One way is to run `diskutil list` before connecting/inserting the disk and run it again after, and rule out any devices that were already present. The one that's only in the “after” output is the one you want.

All of the devices have identifiers like “diskX”, “diskXsY”, or “diskXsYsZ”, where X is the top-level device number and Y and Z are subdevice numbers. “diskXsY” is subdevice Y of device X, and “diskXsYsZ” is a subdevice of a subdevice.

You'll see devices of type “`CD_partition_scheme`” (if the device is a physical CD-ROM), “`Apple_partition_scheme`” (if the device is formatted with an Apple Partition Map), “`Apple_HFS`”, and some others. You want “`Apple_partition_scheme`” if that's present, or “`Apple_HFS`” if not.

(Imaging the `Apple_HFS` device will give you a raw HFS image that impluse can read from directly, but this is not as good for preservation. I recommend imaging the `Apple_partition_scheme` device for a more complete original, and then attaching the image in the subsequent step.)

To image the device, use:

- `hdiutil create -format UDBZ -srcdevice /dev/diskXsY -o "My Image.dmg"`

For CD-ROMs, you'll typically want the first subdevice, diskXs1. For hard drives and most other devices, you're more likely to want the top-level device, diskX. (Each of these is most likely to be the one labeled “`Apple_partition_scheme`” as noted above, though if there's only a bare HFS volume with no partition map, the device may have no type label at all.)

### If you get a “permission denied” error when attempting to image a device

At least with CD-ROMs, macOS has a habit of attaching the subdevices as owned by the `root` user and not by you. This means you can read the top-level device (which is generally not the HFS volume), but not the subdevices (including the one that is the HFS volume).

This means that directly reading from or copying the subdevices as yourself will not work.

If you really want to create a raw image, you can copy the subdevices as root (`sudo cp …`).

My recommendation is to create a UDIF image. You can use `hdiutil create` as described above; hdiutil will prompt for authorization to read from the subdevice as root.

### Attaching an image

If you have a raw image (a file containing bytes directly copied from a device), you can attach it using `hdiutil`:

- `hdiutil attach -nomount -readonly path/to/image.img`

Raw images are the kind you typically use with emulators such as Mini vMac and SheepShaver. Note that if the image uses some other filename extension, such as .dsk, you'll need to rename or hard-link it to .img for the disk image engine to recognize it as a raw image.

If you have a wrapped disk image such as a UDIF (.dmg) image, you will need to attach it. Use this hdiutil command:

- `hdiutil attach -nomount -readonly path/to/image.dmg`

Now the image is attached as a device, and hdiutil has printed a list of one or more devices.

If it gave you only one device path with no further description, use that.

If it gave you a list like:

```
/dev/diskX          	Apple_partition_scheme         	
/dev/diskXs1        	Apple_partition_map            	
/dev/diskXs2        	Apple_Driver43_CD              	
/dev/diskXs3        	Apple_Driver_ATAPI             	
/dev/diskXs4        	Apple_Driver43                 	
/dev/diskXs5        	Apple_Driver_ATAPI             	
/dev/diskXs6        	Apple_Patches                  	
/dev/diskXs7        	Apple_HFS                      	
/dev/diskXs8        	Apple_Driver43                 	
```

You want the one labeled “`Apple_HFS`”.

From this point, you can use the good-old-fashioned `cp` command to copy the indicated device to a raw HFS image file, or you can point impluse to that device directly.

If you get a list of partitions but there is no “`Apple_HFS`” partition, then it's not an HFS volume and impluse cannot do anything with it.

If you get a list with multiple “`Apple_HFS`” partitions, then there are multiple HFS partitions (most likely for a partitioned hard drive). You can create a raw HFS image file from, or use impluse directly with, each partition separately.

### On bare HFS volumes, such as floppies
A storage device that isn't big enough to make sense to partition will often omit the partition map entirely, containing only a bare HFS volume. Often these are floppy disks, or images thereof.

Floppy images for emulators like Mini vMac and SheepShaver will generally be raw images. Floppy images distributed by Apple or other companies are more likely in a wrapped format like NDIF (which inconveniently uses the same .img extension), UDIF (.dmg), Disk Copy 4.2 (no extension), or self-mounting image applications (.smi). Recent versions of macOS have dropped support for pre-UDIF formats, which makes extracting HFS volumes from these much less convenient.

If you followed the instructions above, and then copied an “`Apple_HFS`” subdevice to a regular file, that file is a raw disk image of a bare HFS volume.

If you have a raw bare-HFS disk image file, impluse can read directly from it—no need to attach it.

If you try to use impluse with a file you think is an image of an HFS volume, but it doesn't work, there are several possible reasons why:

- impluse can only read from raw _HFS_ images, where the image file contains only the volume with no partition map around it. impluse does not currently know how to read a partition map. If you have an image file containing a partition map, you'll need to follow the steps above to attach the image and expose any HFS volumes that might be present.
- impluse also does not know how to read wrapped (non-raw) images. You'll need to attach these using the disk images system as described above.
- If you imaged a CD-ROM using its top-level device (`CD_partition_scheme`), that won't work—the data is wrapped in thousands of CD-ROM frame headers. These can be stripped out (with some other program), but it's safer to re-image the disc from the original physical copy.

## Using impluse

Once you have either an attached device that contains an HFS volume (see mentions of “`Apple_HFS`” above) or a raw bare-HFS image file, you can then feed that volume to impluse.

All of these operations are quite fast; they will typically be limited by the speed of I/O (e.g., if you're reading directly from a CD, they will be limited by the speed of your CD drive).

### Conversion to HFS+

- `impluse convert /dev/diskXsY "Insert Name Here-HFS+.img"`

This will output a raw disk image containing a bare HFS+ (Mac OS Extended, as opposed to Mac OS Standard) volume. You should be able to mount the new image immediately if you so choose:

`hdiutil attach -readonly "Insert Name Here-HFS+.img"`

Unlike HFS, HFS+ is still supported on modern macOS, so the image should mount without difficulty and you should be able to browse the volume in the Finder.

You should still keep the HFS original, particularly as impluse is still new and may contain bugs, and certain things may not be implemented yet and may not even be possible to implement.

### Listing volume contents

- `impluse list /dev/diskXsY`

This produces a human-readable hierarchical listing of the entire volume. Emoji are used to indicate whether something is a file (📄) or folder (📁). Note that `list` doesn't look at file type codes or the bundle bit, so even applications will be listed with either the 📄 or 📁 emoji.

### Extracting files, folders and their contents, or the entire volume

- `impluse extract /dev/diskXsY ':'`

':' is a path to the root of the volume; this path will tell `extract` to extract the whole volume as a folder. The folder will have the name of the volume.

Be aware that extracted aliases may be broken and need reconnecting. (Aliases are different from symbolic links, which are path-based; aliases refer to items by ID numbers, and extracted items will have different IDs in the volume  you extract them to than the one they came from.)

- `impluse extract /dev/diskXsY 'Mac OS 9:System Folder:Mac OS ROM'`
- `impluse extract /dev/diskXsY 'Mac OS ROM'`

If you provide a complete absolute path (or a relative path, starting with `:`, which will be interpreted relative to the volume root), impluse will extract that item specifically. If the item is a folder, impluse will extract the folder and all of its contents, including subfolders and their contents.

If you provide only the name of an item, impluse will search the disk for items with that name. If there's only one match, impluse will extract it. If there are multiple matches, impluse will print their paths, and you can pick which one you want.

The above warning about extracting aliases goes double when extracting specific items. Even if it's possible to automatically reconnect an alias if the alias and its destination are both extracted, this isn't possible if the alias is extracted without its destination.

## Things to beware of

### Alias fragility

When you convert a volume to HFS+, expect aliases to be brittle. One of the elements that an alias uses to identify the volume that its target resides on is the volume's format signature, which identifies whether it's HFS, HFS+, or something else entirely. Alias files that refer to other items on the same HFS volume are referring to items on an HFS volume; since the converted volume *isn't* an HFS volume anymore, the alias will no longer match items on it via that search method.

Finder's “Show Original” command will still find the target as long as the path to it hasn't changed. (So, the item hasn't been renamed, its parent directory hasn't been renamed, the item hasn't been moved, etc.) If you rename or move the target, aliases to it will break. If you rename its parent directory, aliases may break (the logic that involves parent directories is too convoluted to explain here; it boils down to a maybe).

This is unavoidable as long as alias files are copied verbatim. They are referring to an HFS volume that isn't present. Working around this would require altering the alias record; I've looked into this, and it's non-trivial, and at any rate it would be optional and off by default since it's an alteration to the files on the volume.

When you extract an alias file, it may or may not work, depending on the circumstances of the extraction. If you simply extract the alias file and its target right next to each other, that won't work unless they were right next to each other on the original volume. If you extract a folder hierarchy or the entire volume, the aliases so extracted may work but be brittle, the same as on a converted volume.

Converting the volume gives the aliases on it one small advantage: Conversion preserves the catalog node IDs of every item on the volume, so an alias referring to file ID such-and-such will still refer to the same file in the converted volume. (*If* it can find that volume, which, see above.) Extracting an item into a pre-existing volume almost certainly gives it a new catalog node ID. An extracted alias file can pretty much *only* successfully match an extracted target by path.

### Generic icons

On modern macOS, custom icons do still seem to show up reliably (as of Monterey), but applications and documents may appear to have generic icons. As far as I can tell, this is because Finder no longer looks at the resources that applications used to use to identify which were their own icons and which belonged to their document types.

Bundle-based applications (including some Carbon apps, such as the last versions of GraphicConverter that supported Mac OS 9) may still work. Also, `'icns'` resources seem to still show up even when palette-based and bitmap icons don't.

All icons should show up as expected if you mount the converted volume on Mac OS 9.

### Item positioning and window frames

If you mount the converted volume on Mac OS 9, window frames and icon positions should still be respected, but modern macOS ignores them.

### Comments

Comments (in Finder's Info window) won't show up in modern macOS.

As far as I can tell, any comments entered in Classic Mac OS are stored somewhere in the desktop database file(s). I don't know specifically where or how, and the format is undocumented.

The modern Finder (which calls them “Spotlight comments” for no reason that has ever been clear) evidently does not look there.

When you convert a volume, the desktop file(s) will be copied over along with everything else, but Finder won't consult them, so your comments will technically exist but not be accessible in the modern world. (Though they should show up if you mount the volume on Mac OS 9.)

When you extract items, impluse would need to extract their comments from the desktop database and then apply them to the extracted copies, and I don't know how to do either of those things.

### Finder reporting sizes inconsistently

I have not been able to get a straight answer from Finder's Info window across all of time and space.

Finder reports sizes in two ways: space taken up “on disk”, which is largely based on files' total physical sizes (i.e., block size times number of blocks), and a precise number of bytes, which is files' total logical sizes (the length of the data fork plus the length of the resource fork).

“on disk” is useless for validating the conversion. Conversion may change the block size to something smaller, in which case many files will use less space “on disk” after conversion—indeed, this was one of HFS+'s selling points.

As for total numbers of bytes, looking at a single file generally works consistently. If you examine the volume, however, examining the same volume on System 6, Mac OS 9, and modern macOS will give you *at least* three different answers.

There is an app for Classic Mac OS called “List Files”, by Alessandro Levi Montalcini, which specifically reports the total lengths of data and resource forks, and gives numbers that agree with impluse's output.

### This tool's purpose is preservation, not repair

impluse's HFS+ converter will not attempt to repair errors in the original volume, unless the errors would make the HFS+ version of the volume unmountable or prevent a successful conversion.

This means that “soft errors” in the original HFS volume will generally be reproduced in the converted volume. These may show up in DiskWarrior but not Disk First Aid/fsck. Among the known cases are:

- forks that are allocated more blocks than they need for their length (DiskWarrior shortens them; impluse does not)
- folders that have an icon file but no custom icon bit (DiskWarrior flags this as an error and will set the custom icon bit in its repair)
- volumes whose root directory has a different creation date from the volume header (DiskWarrior flags this as an error; at least when the volume's creation date is earlier than the root directory, DiskWarrior changes the root directory to match the volume header)

impluse's primary goal is to reproduce the original volume as faithfully as it can. If the original volume had soft errors that aren't fatal, impluse will generally reproduce them in the converted volume.

If you discover an error in the converted volume, try running the tool that reported the error against the original HFS volume. (This includes fsck/Disk Utility, though you may have to run fsck_hfs directly. fsck_hfs still verifies HFS volumes, even though HFS isn't otherwise supported anymore. Use `fsck_hfs -d -D 0xc63` for verbose output, including hex dumps of relevant volume data for some errors.)

If the original volume is clean, meaning that impluse *introduced* an error, then please file a bug.

### Possible bugs

It is, of course, entirely possible that this tool has some failure case I haven't encountered yet. Subtle data loss is the hardest failure mode to detect; it's entirely possible that an extraction or conversion could “succeed” but silently corrupt files in one or more ways, or forget to copy some files.

**I strongly recommend preserving a verbatim copy of your original HFS volume, ideally as a read-only UDIF image. I further recommend keeping the original _physical_ copy if possible.** This is your best bet to avoid permanently losing data because you thought it was migrated and it wasn't.

I disclaim all responsibility for any and all data loss that may occur when you use this tool. You use it at your own risk.
