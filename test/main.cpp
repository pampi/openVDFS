#include <openVDFS.hpp>

using namespace vdfs;

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        VDFS *i = VDFS::getInstance();
        i->addVDFVolume(argv[1]);
        i->getRootEntry()->printAll();

        printf("looking for _WORK/DATA/TEXTURES/FONTS/NOMIP/FONT_10_BOOK.TGA : %s\n",
               (i->fileExist("_WORK/DATA/TEXTURES/FONTS/NOMIP/FONT_10_BOOK.TGA")) ? "FOUND" : "NOT FOUND");
        VDFS::destroyInstance();
    }

    return 0;
}
