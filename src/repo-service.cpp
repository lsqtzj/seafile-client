#include <QTimer>

#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/server-repo.h"
#include "api/requests.h"
#include "repo-service.h"

namespace {

const int kRefreshReposInterval = 1000 * 60 * 5; // 5 min

} // namespace

RepoService* RepoService::singleton_;

RepoService* RepoService::instance()
{
    if (singleton_ == NULL) {
        singleton_ = new RepoService;
    }

    return singleton_;
}

RepoService::RepoService(QObject *parent)
    : QObject(parent)
{
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, SIGNAL(timeout()), this, SLOT(refresh()));
    list_repo_req_ = NULL;
    in_refresh_ = false;
}

void RepoService::start()
{
    refresh_timer_->start(kRefreshReposInterval);
    refresh();
}

void RepoService::refresh()
{
    if (in_refresh_) {
        return;
    }

    AccountManager *account_mgr = seafApplet->accountManager();

    const std::vector<Account>& accounts = seafApplet->accountManager()->accounts();
    if (accounts.empty()) {
        in_refresh_ = false;
        return;
    }

    in_refresh_ = true;

    if (list_repo_req_) {
        delete list_repo_req_;
    }

    list_repo_req_ = new ListReposRequest(accounts[0]);

    connect(list_repo_req_, SIGNAL(success(const std::vector<ServerRepo>&)),
            this, SLOT(onRefreshSuccess(const std::vector<ServerRepo>&)));

    connect(list_repo_req_, SIGNAL(failed(const ApiError&)),
            this, SLOT(onRefreshFailed(const ApiError&)));
    list_repo_req_->send();
}

void RepoService::onRefreshSuccess(const std::vector<ServerRepo>& repos)
{
    in_refresh_ = false;

    server_repos_ = repos;

    emit refreshSuccess(repos);
}

void RepoService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    emit refreshFailed(error);
}

void RepoService::refresh(bool force)
{
    if (!force || !in_refresh_) {
        refresh();
        return;
    }

    // Abort the current request and send another
    in_refresh_ = false;
    refresh();
}
