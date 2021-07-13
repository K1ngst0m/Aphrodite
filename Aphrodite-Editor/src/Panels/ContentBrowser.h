//
// Created by npchitman on 6/30/21.
//

#ifndef APHRODITE_CONTENTBROWSER_H
#define APHRODITE_CONTENTBROWSER_H

#include <filesystem>

namespace Aph::Editor{
    class ContentBrowser {
    public:
        ContentBrowser();

        void OnUIRender();

    private:
        std::filesystem::path m_CurrentDirectory;
    };
}


#endif//APHRODITE_CONTENTBROWSER_H
