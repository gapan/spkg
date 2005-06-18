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
#include "untgz.h"

typedef struct Package_t Package;
typedef struct Packages_t Packages;
typedef struct PackagesIter_t PackagesIter;
typedef struct File_t File;
typedef struct Files_t Files;
typedef struct FilesIter_t FilesIter;
typedef struct Untgz_t Untgz;

/* note that these objects are just refereneces to real data.
   object duplication is not supported. */

struct Package_t
{
  PyObject_HEAD
  struct db_pkg* pkg; /* package entry */
  Packages* pkgs;     /* if package is from Packages object */
};

struct Packages_t
{
  PyObject_HEAD
  GSList* pkgs; /* packages list */
};

struct PackagesIter_t
{
  PyObject_HEAD
  GSList* cur; /* current package */
  Packages* pkgs; /* Packages to which this PackagesIter belongs */
};

struct File_t
{
  PyObject_HEAD
  struct db_file* file; /* file entry */
  PyObject* parent; /* Packages to which this PackagesIter belongs */
};

struct Files_t
{
  PyObject_HEAD
  GSList* files; /* files list */
  PyObject* parent; /* Packages to which this PackagesIter belongs */
};

struct FilesIter_t
{
  PyObject_HEAD
  GSList* cur;
  Files* files; /* Files to which this FilesIter belongs */
};

struct Untgz_t
{
  PyObject_HEAD
  struct untgz_state* s;
};

extern PyObject* PySpkgErrorObject;

extern PyTypeObject File_Type;
extern PyTypeObject Files_Type;
extern PyTypeObject FilesIter_Type;
extern PyTypeObject Package_Type;
extern PyTypeObject Packages_Type;
extern PyTypeObject PackagesIter_Type;
extern PyTypeObject Untgz_Type;

extern Package* newPackage(struct db_pkg* pkg, Packages* pkgs);
extern Packages* newPackages(GSList* pkgs);
extern PackagesIter* newPackagesIter(Packages* pkgs);
extern File* newFile(struct db_file* file, PyObject* parent);
extern Files* newFiles(GSList* files, PyObject* parent);
extern FilesIter* newFilesIter(Files* files);
extern Untgz* newUntgz(struct untgz_state* s);

#define File_Check(v) ((v)->ob_type == &File_Type)
#define Files_Check(v) ((v)->ob_type == &Files_Type)
#define Package_Check(v) ((v)->ob_type == &Package_Type)
#define Packages_Check(v) ((v)->ob_type == &Packages_Type)
#define Untgz_Check(v) ((v)->ob_type == &Untgz_Type)

#include "pyspkg-meth.h"

#endif
