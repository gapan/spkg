/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#ifndef __PYSPKG_H
#define __PYSPKG_H

#include <Python.h>
#include "pkgdb.h"

typedef struct Package_t Package;
typedef struct Packages_t Packages;
typedef struct PackagesIter_t PackagesIter;
typedef struct File_t File;
typedef struct Files_t Files;
typedef struct FilesIter_t FilesIter;

struct Package_t
{
  PyObject_HEAD
  struct db_pkg* p;
  int free;
};

struct Packages_t
{
  PyObject_HEAD
  GSList* pkgs;
};

struct PackagesIter_t
{
  PyObject_HEAD
  GSList* cur;
  Packages* pkgs;
};

struct File_t
{
  PyObject_HEAD
  struct db_file* p;
  int free;
};

struct Files_t
{
  PyObject_HEAD
  GSList* files;
};

struct FilesIter_t
{
  PyObject_HEAD
  GSList* cur;
  Files* files;
};

extern PyObject* PySpkgErrorObject;

extern PyTypeObject File_Type;
extern PyTypeObject Files_Type;
extern PyTypeObject FilesIter_Type;
extern PyTypeObject Package_Type;
extern PyTypeObject Packages_Type;
extern PyTypeObject PackagesIter_Type;

extern Package* newPackage(struct db_pkg* p, int free);
extern Packages* newPackages(GSList* pkgs);
extern PackagesIter* newPackagesIter(Packages* p);
extern File* newFile(struct db_file* p);
extern Files* newFiles(GSList* files);
extern FilesIter* newFilesIter(Files* p);

#define File_Check(v) ((v)->ob_type == &File_Type)
#define Files_Check(v) ((v)->ob_type == &Files_Type)
#define Package_Check(v) ((v)->ob_type == &Package_Type)
#define Packages_Check(v) ((v)->ob_type == &Packages_Type)

#include "pyspkg-meth.h"

#endif
