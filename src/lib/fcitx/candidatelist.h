/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CANDIDATELIST_H_
#define _FCITX_CANDIDATELIST_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx/candidateaction.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/text.h>

namespace fcitx {

class InputContext;
class PageableCandidateList;
class BulkCandidateList;
class ModifiableCandidateList;
class CursorMovableCandidateList;
class CursorModifiableCandidateList;
class BulkCursorCandidateList;
class ActionableCandidateList;

class CandidateListPrivate;

enum class CandidateLayoutHint { NotSet, Vertical, Horizontal };

class CandidateWordPrivate;

/// Base class of candidate word.
class FCITXCORE_EXPORT CandidateWord {
public:
    CandidateWord(Text text = {});
    virtual ~CandidateWord();
    /**
     * Called when candidate is selected by user.
     *
     * @param inputContext the associated input context for the candidate.
     */
    virtual void select(InputContext *inputContext) const = 0;

    const Text &text() const;
    /**
     * Whether the candidate is only a place holder.
     *
     * If candidate is a place holder, it will not be displayed by UI, but it
     * will still take one place in the candidate list.
     */
    bool isPlaceHolder() const;
    bool hasCustomLabel() const;
    const Text &customLabel() const;
    /**
     * Return comment corresponding to the candidate.
     *
     * @return value of comment.
     * @since 5.1.9
     */
    const Text &comment() const;
    /**
     * Return text with comment.
     *
     * @param separator separator between text and comment.
     * @return value of comment.
     * @since 5.1.9
     */
    Text textWithComment(std::string separator = " ") const;

protected:
    void setText(Text text);
    void setPlaceHolder(bool placeHolder);
    void resetCustomLabel();
    void setCustomLabel(Text text);
    void setComment(Text comment);

private:
    std::unique_ptr<CandidateWordPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(CandidateWord);
};

// basic stuff
class FCITXCORE_EXPORT CandidateList {
public:
    CandidateList();
    virtual ~CandidateList();

    virtual const Text &label(int idx) const = 0;
    virtual const CandidateWord &candidate(int idx) const = 0;
    virtual int size() const = 0;
    virtual int cursorIndex() const = 0;
    virtual CandidateLayoutHint layoutHint() const = 0;

    bool empty() const;

    PageableCandidateList *toPageable() const;
    BulkCandidateList *toBulk() const;
    ModifiableCandidateList *toModifiable() const;
    CursorMovableCandidateList *toCursorMovable() const;
    CursorModifiableCandidateList *toCursorModifiable() const;
    BulkCursorCandidateList *toBulkCursor() const;
    ActionableCandidateList *toActionable() const;

protected:
    void setPageable(PageableCandidateList *list);
    void setBulk(BulkCandidateList *list);
    void setModifiable(ModifiableCandidateList *list);
    void setCursorMovable(CursorMovableCandidateList *list);
    void setCursorModifiable(CursorModifiableCandidateList *list);
    void setBulkCursor(BulkCursorCandidateList *list);
    void setActionable(ActionableCandidateList *list);

private:
    std::unique_ptr<CandidateListPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(CandidateList);
};

// useful for regular input method
class FCITXCORE_EXPORT PageableCandidateList {
public:
    // Need for paging
    virtual bool hasPrev() const = 0;
    virtual bool hasNext() const = 0;
    virtual void prev() = 0;
    virtual void next() = 0;

    virtual bool usedNextBefore() const = 0;

    // Following are optional.
    virtual int totalPages() const { return -1; }
    virtual int currentPage() const { return -1; }
    virtual void setPage(int /*unused*/) {}
};

class FCITXCORE_EXPORT CursorMovableCandidateList {
public:
    virtual void prevCandidate() = 0;
    virtual void nextCandidate() = 0;
};

class FCITXCORE_EXPORT CursorModifiableCandidateList {
public:
    virtual void setCursorIndex(int index) = 0;
};

// useful for virtual keyboard
class FCITXCORE_EXPORT BulkCandidateList {
public:
    /**
     * If idx is out of range, it may raise exception. Catching the exception is
     * useful to iterate over all candidate list for candidate list has no total
     * size.
     */
    virtual const CandidateWord &candidateFromAll(int idx) const = 0;
    /**
     * It's possible for this function to return -1 if the implement has no
     * clear number how many candidates are available.
     */
    virtual int totalSize() const = 0;
};

// useful for module other than input method
class FCITXCORE_EXPORT ModifiableCandidateList : public BulkCandidateList {
public:
    // All index used there are global index
    virtual void insert(int idx, std::unique_ptr<CandidateWord> word) = 0;
    virtual void remove(int idx) = 0;
    virtual void replace(int idx, std::unique_ptr<CandidateWord> word) = 0;
    virtual void move(int from, int to) = 0;

    void append(std::unique_ptr<CandidateWord> word) {
        insert(totalSize(), std::move(word));
    }

    template <typename CandidateWordType, typename... Args>
    void append(Args &&...args) {
        append(
            std::make_unique<CandidateWordType>(std::forward<Args>(args)...));
    }
};

class FCITXCORE_EXPORT DisplayOnlyCandidateWord : public CandidateWord {
public:
    DisplayOnlyCandidateWord(Text text) : CandidateWord(std::move(text)) {}

    void select(InputContext * /*inputContext*/) const override {}
};

class FCITXCORE_EXPORT BulkCursorCandidateList {
public:
    virtual int globalCursorIndex() const = 0;
    virtual void setGlobalCursorIndex(int index) = 0;
};

/**
 * Interface for trigger actions on candidates.
 *
 * @since 5.1.10
 */
class FCITXCORE_EXPORT ActionableCandidateList {
public:
    virtual ~ActionableCandidateList();

    /**
     * Check whether this candidate has action.
     *
     * This function should be fast and guarantee that candidateActions return a
     * not empty vector.
     */
    virtual bool hasAction(const CandidateWord &candidate) const = 0;

    /**
     * Return a list of actions.
     */
    virtual std::vector<CandidateAction>
    candidateActions(const CandidateWord &candidate) const = 0;

    /**
     * Trigger the action based on the index returned from candidateActions.
     */
    virtual void triggerAction(const CandidateWord &candidate, int id) = 0;
};

class DisplayOnlyCandidateListPrivate;

class FCITXCORE_EXPORT DisplayOnlyCandidateList : public CandidateList {
public:
    DisplayOnlyCandidateList();
    ~DisplayOnlyCandidateList();

    void setContent(const std::vector<std::string> &content);
    void setContent(std::vector<Text> content);
    void setLayoutHint(CandidateLayoutHint hint);
    void setCursorIndex(int index);

    // CandidateList
    const fcitx::Text &label(int idx) const override;
    const CandidateWord &candidate(int idx) const override;
    int cursorIndex() const override;
    int size() const override;
    CandidateLayoutHint layoutHint() const override;

private:
    std::unique_ptr<DisplayOnlyCandidateListPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(DisplayOnlyCandidateList);
};

class CommonCandidateListPrivate;

enum class CursorPositionAfterPaging { SameAsLast, DonotChange, ResetToFirst };

/**
 * A common simple candidate list that serves most of the purpose.
 */
class FCITXCORE_EXPORT CommonCandidateList : public CandidateList,
                                             public PageableCandidateList,
                                             public ModifiableCandidateList,
                                             public CursorMovableCandidateList {
public:
    CommonCandidateList();
    ~CommonCandidateList();

    void clear();

    /**
     * Set the label of candidate list.
     *
     * The labels less than 10 will be automatically filled with to empty ones
     * up to 10 to be more error prone.
     *
     * @param labels list of labels.
     *
     * @since 5.0.4
     */
    void setLabels(const std::vector<std::string> &labels = {});

    /**
     * Set the label of candidate list by key.
     *
     * @param keyList list of selection key
     */
    void setSelectionKey(const KeyList &keyList);

    void setPageSize(int size);
    int pageSize() const;
    void setLayoutHint(CandidateLayoutHint hint);
    void setGlobalCursorIndex(int index);
    /**
     * Return Global cursor index.
     *
     * -1 means it is not selected.
     *
     * @return cursor index.
     * @since 5.0.4
     */
    int globalCursorIndex() const;

    /**
     * Set cursor index on current page.
     *
     * @param index index on current page;
     * @since 5.1.9
     */
    void setCursorIndex(int index);

    // CandidateList
    const fcitx::Text &label(int idx) const override;
    const CandidateWord &candidate(int idx) const override;
    int cursorIndex() const override;
    int size() const override;

    // PageableCandidateList
    bool hasPrev() const override;
    bool hasNext() const override;
    void prev() override;
    void next() override;

    bool usedNextBefore() const override;

    int totalPages() const override;
    int currentPage() const override;
    void setPage(int page) override;

    CandidateLayoutHint layoutHint() const override;

    // BulkCandidateList
    const CandidateWord &candidateFromAll(int idx) const override;
    int totalSize() const override;

    // ModifiableCandidateList
    void insert(int idx, std::unique_ptr<CandidateWord> word) override;
    void remove(int idx) override;
    void replace(int idx, std::unique_ptr<CandidateWord> word) override;
    void move(int from, int to) override;

    // CursorMovableCandidateList
    void prevCandidate() override;
    void nextCandidate() override;

    // A simple switch to change the behavior of prevCandidate and nextCandidate
    void setCursorIncludeUnselected(bool);
    void setCursorKeepInSamePage(bool);
    void setCursorPositionAfterPaging(CursorPositionAfterPaging afterPaging);

    /**
     * Set an optional implemenation of actionable candidate list
     *
     * @since 5.1.10
     */
    void setActionableImpl(std::unique_ptr<ActionableCandidateList> actionable);

private:
    void fixAfterUpdate();
    void moveCursor(bool prev);

    std::unique_ptr<CommonCandidateListPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(CommonCandidateList);
};
} // namespace fcitx

#endif // _FCITX_CANDIDATELIST_H_
