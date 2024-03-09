//
//  ImpDehydratedItem.h
//  impluse-hfs
//
//  Created by Peter Hosey on 2022-12-02.
//

#import <Foundation/Foundation.h>

#import "ImpForkUtilities.h"

@class ImpSourceVolume;
@class ImpDestinationVolume;

typedef NS_ENUM(NSUInteger, ImpDehydratedItemType) {
	ImpDehydratedItemTypeFile = 1,
	ImpDehydratedItemTypeFolder,
	ImpDehydratedItemTypeVolume,
};

///A dehydrated item is a file or folder that exists within a source volume.
@interface ImpDehydratedItem : NSObject

///Create a dehydrated item object that references a given HFS catalog. The initializer will populate the object's properties with the catalog's data for the given catalog node ID.
- (instancetype _Nonnull) initWithHFSVolume:(ImpSourceVolume *_Nonnull const)hfsVol
	catalogNodeID:(HFSCatalogNodeID const)cnid
	key:(struct HFSCatalogKey const *_Nonnull const)key
	fileRecord:(struct HFSCatalogFile const *_Nonnull const)fileRec;

///Create a dehydrated item object that references a given HFS catalog. The initializer will populate the object's properties with the catalog's data for the given catalog node ID.
- (instancetype _Nonnull) initWithHFSVolume:(ImpSourceVolume *_Nonnull const)hfsVol	catalogNodeID:(HFSCatalogNodeID const)cnid
	key:(struct HFSCatalogKey const *_Nonnull const)key
	folderRecord:(struct HFSCatalogFolder const *_Nonnull const)folderRec;

///Create a dehydrated item object that references a given HFS+ catalog. The initializer will populate the object's properties with the catalog's data for the given catalog node ID.
- (instancetype _Nonnull) initWithHFSPlusVolume:(ImpDestinationVolume *_Nonnull const)hfsVol
	catalogNodeID:(HFSCatalogNodeID const)cnid
	key:(struct HFSPlusCatalogKey const *_Nonnull const)key
	fileRecord:(struct HFSPlusCatalogFile const *_Nonnull const)fileRec;

///Create a dehydrated item object that references a given HFS+ catalog. The initializer will populate the object's properties with the catalog's data for the given catalog node ID.
- (instancetype _Nonnull) initWithHFSPlusVolume:(ImpDestinationVolume *_Nonnull const)hfsVol
	catalogNodeID:(HFSCatalogNodeID const)cnid
	key:(struct HFSPlusCatalogKey const *_Nonnull const)key
	folderRecord:(struct HFSPlusCatalogFolder const *_Nonnull const)folderRec;

@property(weak) ImpSourceVolume * _Nullable sourceVolume;
@property HFSCatalogNodeID catalogNodeID;
@property ImpDehydratedItemType type;
///True for folders and volumes. False for files.
@property(nonatomic, readonly) bool isDirectory;

@property(copy) NSData *_Nonnull hfsCatalogKeyData;
@property(copy) NSData *_Nullable hfsFileCatalogRecordData;
@property(copy) NSData *_Nullable hfsFolderCatalogRecordData;

@property(nonatomic) HFSCatalogNodeID parentFolderID;

///Convert the item's name from the HFS catalog using its assigned encoding into a modern Unicode name.
- (NSString *_Nonnull const) name;
///Convert the item's createDate from the HFS catalog to the modern epoch.
- (NSDate *_Nonnull const) creationDate;
///Convert the item's modifyDate from the HFS catalog to the modern epoch.
- (NSDate *_Nonnull const) modificationDate;
///The item's four-byte type code. Only valid for files; returns 0 for folders.
@property(nonatomic, readonly) OSType fileTypeCode;
///The item's four-byte creator code (also called its signature when the item is an application). Only valid for files; returns 0 for folders.
@property(nonatomic, readonly) OSType creatorCode;

///The logical length of the file's data fork as a number of bytes. Returns 0 for folders.
@property(nonatomic, readonly) u_int64_t dataForkLogicalLength;
///The logical length of the file's resource fork as a number of bytes. Returns 0 for folders.
@property(nonatomic, readonly) u_int64_t resourceForkLogicalLength;

///The short version string from the item's 'vers' resource ID 1, if such a resource exists. Returns nil if there is no such resource or if the item is a folder. The string may be empty.
- (NSString *_Nullable const) shortVersionString;
///The short version string from the item's 'vers' resource ID 1, if such a resource exists. Returns nil if there is no such resource or if the item is a folder. The string may be empty.
- (NSString *_Nullable const) versionStringFromVersionNumber;

///Reconstruct the path to the item from the volume's catalog. Returns an array of item names, starting with the volume name, that, if joined by colons, will form an HFS path.
- (NSArray <NSString *> *_Nonnull const) path;

///Returns an NSData containing the data from either the data fork or the resource fork. Returns nil if whichFork is invalid or the item is not a file.
- (NSData *_Nullable const) rehydrateForkContents:(ImpForkType)whichFork;

///Create a real file or folder with the same contents and (as much as possible) metadata as the dehydrated item. Folders get rehydrated recursively, with all of their sub-items. Note that this must be the URL of the item to be created (i.e., parent directory + nameFromEncoding:).
- (bool) rehydrateAtRealWorldURL:(NSURL *_Nonnull const)realWorldURL error:(NSError *_Nullable *_Nonnull const)outError;

///Create a real file or folder with the same contents and (as much as possible) metadata as the dehydrated item. Folders get rehydrated recursively, with all of their sub-items. The item's filename will be converted to Unicode and appended to realWorldParentURL.
- (bool) rehydrateIntoRealWorldDirectoryAtURL:(NSURL *_Nonnull const)realWorldParentURL error:(NSError *_Nullable *_Nonnull const)outError;

#pragma mark Directory trees

///From an HFS volume, create a hierarchy of ImpDehydratedItems representing the files and folders on that volume. Returns the root directory.
+ (instancetype _Nonnull) rootDirectoryOfHFSVolume:(ImpSourceVolume *_Nonnull const)hfsVol;

///Only present on dehydrated folders created by rootDirectoryOfHFSVolume:. Contains all of the files and folders that are immediate children of this folder (i.e., whose parent is this folder).
@property(nullable, nonatomic, readonly, copy) NSArray <ImpDehydratedItem *> *children;
- (NSUInteger) countOfChildren;
- (void) addChildrenObject:(ImpDehydratedItem *_Nonnull const)object;

///Walk the directory hierarchy breadth-first starting from this directory, calling the block for each item encountered. Use the depth value for indentation. Results undefined if you call this directly upon a dehydrated file (rather than a folder or volume).
- (void) walkBreadthFirst:(void (^_Nonnull const)(NSUInteger const depth, ImpDehydratedItem *_Nonnull const item))block;

///Used by ImpHFSLister to print directory hierarchies to the terminal. This is, effectively, the real implementation of the list command.
- (void) printDirectoryHierarchy_asPaths:(bool)printAbsolutePaths;

@end
