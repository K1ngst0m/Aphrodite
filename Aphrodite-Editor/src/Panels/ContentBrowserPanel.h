//
// Created by npchitman on 6/30/21.
//

#ifndef APHRODITE_CONTENTBROWSERPANEL_H
#define APHRODITE_CONTENTBROWSERPANEL_H

#include <filesystem>

namespace Aph{
    class ContentBrowserPanel {
    public:
        ContentBrowserPanel();

        void OnImGuiRender();

    private:
        std::filesystem::path m_CurrentDirectory;
    };
}


#endif//APHRODITE_CONTENTBROWSERPANEL_H
