#import <Cocoa/Cocoa.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "application/Match.h"
#include "infrastructure/MatchFactory.h"

using battleship::application::Match;
using battleship::domain::Board;
using battleship::domain::Cell;
using battleship::domain::Turn;

using BPoint = battleship::domain::Point;

enum class AppScreen {
    MainMenu,
    Game
};

static NSString* ToNSString(const std::string& text) {
    return [NSString stringWithUTF8String:text.c_str()];
}

static NSColor* RGB(CGFloat r, CGFloat g, CGFloat b, CGFloat a = 1.0) {
    return [NSColor colorWithCalibratedRed:r / 255.0
                                     green:g / 255.0
                                      blue:b / 255.0
                                     alpha:a];
}

static NSFont* RoundedFont(CGFloat size, NSFontWeight weight) {
    NSFont* preferred = [NSFont fontWithName:@"SF Pro Rounded" size:size];
    if (preferred != nil) {
        return [[NSFontManager sharedFontManager] convertFont:preferred toHaveTrait:0];
    }
    return [NSFont systemFontOfSize:size weight:weight];
}

struct BoardLayout {
    NSRect panelRect;
    NSRect gridRect;
    CGFloat cellSize = 0.0;
    CGFloat gap = 0.0;
};

static BoardLayout MakeBoardLayout(NSRect bounds, bool leftBoard) {
    const CGFloat sideMargin = 36.0;
    const CGFloat topInset = 122.0;
    const CGFloat interBoardGap = 28.0;
    const CGFloat panelHeight = std::min(520.0, bounds.size.height - 300.0);
    const CGFloat panelWidth = (bounds.size.width - sideMargin * 2.0 - interBoardGap) / 2.0;
    const CGFloat panelX = leftBoard ? sideMargin : sideMargin + panelWidth + interBoardGap;
    const CGFloat panelY = topInset;
    const CGFloat gap = 6.0;
    const CGFloat labelWidth = 30.0;
    const CGFloat labelHeight = 28.0;
    const CGFloat horizontalPadding = 34.0;
    const CGFloat topPadding = 92.0;
    const CGFloat bottomPadding = 30.0;
    const CGFloat availableWidth = panelWidth - horizontalPadding * 2.0 - labelWidth;
    const CGFloat availableHeight = panelHeight - topPadding - bottomPadding - labelHeight;
    const CGFloat cellSize = floor((std::min(availableWidth, availableHeight) - gap * 9.0) / 10.0);
    const CGFloat gridWidth = cellSize * 10.0 + gap * 9.0;
    const CGFloat gridX = panelX + (panelWidth - (labelWidth + 12.0 + gridWidth)) / 2.0 + labelWidth + 12.0;
    const CGFloat gridY = panelY + topPadding + labelHeight;

    BoardLayout layout;
    layout.panelRect = NSMakeRect(panelX, panelY, panelWidth, panelHeight);
    layout.gridRect = NSMakeRect(gridX, gridY, gridWidth, gridWidth);
    layout.cellSize = cellSize;
    layout.gap = gap;
    return layout;
}

static NSRect RectForCell(const BoardLayout& layout, BPoint point) {
    return NSMakeRect(layout.gridRect.origin.x + point.col * (layout.cellSize + layout.gap),
                      layout.gridRect.origin.y + point.row * (layout.cellSize + layout.gap),
                      layout.cellSize,
                      layout.cellSize);
}

static std::optional<BPoint> PointFromLocation(NSPoint location, const BoardLayout& layout) {
    if (!NSPointInRect(location, layout.gridRect)) {
        return std::nullopt;
    }

    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            BPoint point{row, col};
            if (NSPointInRect(location, RectForCell(layout, point))) {
                return point;
            }
        }
    }

    return std::nullopt;
}

static void FillRoundedRect(NSRect rect, CGFloat radius, NSColor* color) {
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
    [color setFill];
    [path fill];
}

static void StrokeRoundedRect(NSRect rect, CGFloat radius, NSColor* color, CGFloat width) {
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
    [color setStroke];
    [path setLineWidth:width];
    [path stroke];
}

@class BattleshipController;

@interface BattleshipView : NSView
- (instancetype)initWithFrame:(NSRect)frame controller:(BattleshipController*)controller;
- (void)refreshControls;
@end

@interface BattleshipController : NSObject <NSApplicationDelegate>
- (const Match&)game;
- (BOOL)isMainMenu;
- (BOOL)canInteract;
- (BOOL)canShuffle;
- (BOOL)canStartBattle;
- (BOOL)isPlacingFleet;
- (BOOL)isHorizontalPlacement;
- (int)nextShipLength;
- (void)handlePlayerCellClick:(BPoint)point;
- (void)handleEnemyCellClick:(BPoint)point;
- (void)startNewGame:(id)sender;
- (void)shuffleFleet:(id)sender;
- (void)clearFleet:(id)sender;
- (void)rotatePlacement:(id)sender;
- (void)startBattle:(id)sender;
- (void)showMainMenu:(id)sender;
- (void)saveGame:(id)sender;
- (void)loadGame:(id)sender;
- (void)exitGame:(id)sender;
@end

@implementation BattleshipView {
    __weak BattleshipController* _controller;
    NSTrackingArea* _trackingArea;
    std::optional<BPoint> _hoveredPlayerCell;
    std::optional<BPoint> _hoveredEnemyCell;
    NSButton* _newGameButton;
    NSButton* _continueButton;
    NSButton* _exitButton;
    NSButton* _menuButton;
    NSButton* _rotateButton;
    NSButton* _startBattleButton;
    NSButton* _shuffleButton;
    NSButton* _clearButton;
    NSButton* _saveButton;
    NSButton* _loadButton;
}

- (instancetype)initWithFrame:(NSRect)frame controller:(BattleshipController*)controller {
    self = [super initWithFrame:frame];
    if (self == nil) {
        return nil;
    }

    _controller = controller;
    self.wantsLayer = YES;

    _newGameButton = [self makeButtonWithTitle:@"Новая игра" action:@selector(startNewGame:)];
    _continueButton = [self makeButtonWithTitle:@"Продолжить" action:@selector(loadGame:)];
    _exitButton = [self makeButtonWithTitle:@"Выйти" action:@selector(exitGame:)];
    _menuButton = [self makeButtonWithTitle:@"Меню" action:@selector(showMainMenu:)];
    _rotateButton = [self makeButtonWithTitle:@"Повернуть" action:@selector(rotatePlacement:)];
    _startBattleButton = [self makeButtonWithTitle:@"Начать бой" action:@selector(startBattle:)];
    _shuffleButton = [self makeButtonWithTitle:@"Автофлот" action:@selector(shuffleFleet:)];
    _clearButton = [self makeButtonWithTitle:@"Очистить" action:@selector(clearFleet:)];
    _saveButton = [self makeButtonWithTitle:@"Сохранить" action:@selector(saveGame:)];
    _loadButton = [self makeButtonWithTitle:@"Загрузить" action:@selector(loadGame:)];
    [self addSubview:_newGameButton];
    [self addSubview:_continueButton];
    [self addSubview:_exitButton];
    [self addSubview:_menuButton];
    [self addSubview:_rotateButton];
    [self addSubview:_startBattleButton];
    [self addSubview:_shuffleButton];
    [self addSubview:_clearButton];
    [self addSubview:_saveButton];
    [self addSubview:_loadButton];
    [self refreshControls];
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (NSButton*)makeButtonWithTitle:(NSString*)title action:(SEL)action {
    NSButton* button = [NSButton buttonWithTitle:title target:_controller action:action];
    button.bordered = NO;
    button.wantsLayer = YES;
    button.layer.cornerRadius = 14.0;
    button.font = [NSFont systemFontOfSize:15.0 weight:NSFontWeightSemibold];
    button.contentTintColor = [NSColor whiteColor];
    return button;
}

- (void)layout {
    [super layout];

    if ([_controller isMainMenu]) {
        const CGFloat width = 260.0;
        const CGFloat height = 50.0;
        const CGFloat x = (self.bounds.size.width - width) / 2.0;
        const CGFloat y = 330.0;
        _newGameButton.frame = NSMakeRect(x, y, width, height);
        _continueButton.frame = NSMakeRect(x, y + 68.0, width, height);
        _exitButton.frame = NSMakeRect(x, y + 136.0, width, height);
        return;
    }

    const CGFloat buttonHeight = 44.0;
    const CGFloat rightMargin = 36.0;
    const CGFloat top = 38.0;
    CGFloat x = self.bounds.size.width - rightMargin;
    _menuButton.frame = NSMakeRect((x -= 96.0), top, 96.0, buttonHeight);
    x -= 12.0;
    _saveButton.frame = NSMakeRect((x -= 116.0), top, 116.0, buttonHeight);
    x -= 12.0;
    _startBattleButton.frame = NSMakeRect((x -= 128.0), top, 128.0, buttonHeight);
    x -= 12.0;
    _rotateButton.frame = NSMakeRect((x -= 116.0), top, 116.0, buttonHeight);
    x -= 12.0;
    _shuffleButton.frame = NSMakeRect((x -= 116.0), top, 116.0, buttonHeight);
    x -= 12.0;
    _clearButton.frame = NSMakeRect((x -= 112.0), top, 112.0, buttonHeight);
}

- (void)refreshControls {
    BOOL isMenu = [_controller isMainMenu];
    _newGameButton.hidden = !isMenu;
    _continueButton.hidden = !isMenu;
    _exitButton.hidden = !isMenu;
    _menuButton.hidden = isMenu;
    _rotateButton.hidden = isMenu;
    _startBattleButton.hidden = isMenu;
    _shuffleButton.hidden = isMenu;
    _clearButton.hidden = isMenu;
    _saveButton.hidden = isMenu;
    _loadButton.hidden = YES;

    if (isMenu) {
        _newGameButton.layer.backgroundColor = RGB(232, 84, 62).CGColor;
        _continueButton.layer.backgroundColor = RGB(42, 139, 126).CGColor;
        _exitButton.layer.backgroundColor = RGB(77, 87, 108).CGColor;
        return;
    }

    BOOL canShuffle = [_controller canShuffle];
    BOOL isPlacing = [_controller isPlacingFleet];
    _shuffleButton.enabled = canShuffle;
    _shuffleButton.layer.backgroundColor = (canShuffle ? RGB(38, 159, 138).CGColor : RGB(70, 87, 112).CGColor);

    _clearButton.enabled = canShuffle;
    _clearButton.layer.backgroundColor = (canShuffle ? RGB(106, 118, 145).CGColor : RGB(70, 87, 112).CGColor);

    _rotateButton.enabled = isPlacing;
    _rotateButton.layer.backgroundColor = (isPlacing ? RGB(64, 126, 191).CGColor : RGB(70, 87, 112).CGColor);
    _rotateButton.title = [_controller isHorizontalPlacement] ? @"Горизонтально" : @"Вертикально";

    _startBattleButton.enabled = [_controller canStartBattle];
    _startBattleButton.layer.backgroundColor = ([_controller canStartBattle] ? RGB(233, 94, 76).CGColor : RGB(70, 87, 112).CGColor);

    _menuButton.enabled = YES;
    _menuButton.layer.backgroundColor = RGB(74, 86, 108).CGColor;

    _saveButton.enabled = YES;
    _saveButton.layer.backgroundColor = RGB(47, 115, 181).CGColor;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    if (_trackingArea != nil) {
        [self removeTrackingArea:_trackingArea];
    }

    _trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)mouseMoved:(NSEvent*)event {
    if ([_controller isMainMenu]) {
        return;
    }

    NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    BoardLayout playerLayout = MakeBoardLayout(self.bounds, true);
    BoardLayout enemyLayout = MakeBoardLayout(self.bounds, false);
    std::optional<BPoint> nextPlayerHover = PointFromLocation(location, playerLayout);
    std::optional<BPoint> nextHover = PointFromLocation(location, enemyLayout);
    if (![_controller isPlacingFleet]) {
        nextPlayerHover.reset();
    }
    if (![_controller canInteract]) {
        nextHover.reset();
    }

    if (_hoveredEnemyCell != nextHover || _hoveredPlayerCell != nextPlayerHover) {
        _hoveredEnemyCell = nextHover;
        _hoveredPlayerCell = nextPlayerHover;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDown:(NSEvent*)event {
    if ([_controller isMainMenu]) {
        return;
    }

    NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    if ([_controller isPlacingFleet]) {
        BoardLayout playerLayout = MakeBoardLayout(self.bounds, true);
        std::optional<BPoint> point = PointFromLocation(location, playerLayout);
        if (point.has_value()) {
            [_controller handlePlayerCellClick:*point];
        }
        return;
    }

    if ([_controller canInteract]) {
        BoardLayout layout = MakeBoardLayout(self.bounds, false);
        std::optional<BPoint> point = PointFromLocation(location, layout);
        if (point.has_value()) {
            [_controller handleEnemyCellClick:*point];
        }
    }
}

- (void)drawBackgroundInRect:(NSRect)dirtyRect {
    NSGradient* gradient = [[NSGradient alloc] initWithColors:@[
        RGB(10, 17, 30),
        RGB(20, 35, 57),
        RGB(31, 54, 82)
    ]];
    [gradient drawInRect:dirtyRect angle:90.0];

    FillRoundedRect(NSMakeRect(0.0, 0.0, self.bounds.size.width, 168.0), 0.0, RGB(255, 255, 255, 0.035));
    FillRoundedRect(NSMakeRect(0.0, self.bounds.size.height - 150.0, self.bounds.size.width, 150.0), 0.0, RGB(8, 14, 26, 0.22));

    [RGB(171, 203, 235, 0.055) setStroke];
    for (CGFloat x = 36.0; x < self.bounds.size.width; x += 64.0) {
        NSBezierPath* line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(x, 0.0)];
        [line lineToPoint:NSMakePoint(x, self.bounds.size.height)];
        [line setLineWidth:1.0];
        [line stroke];
    }
    for (CGFloat y = 132.0; y < self.bounds.size.height; y += 64.0) {
        NSBezierPath* line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(0.0, y)];
        [line lineToPoint:NSMakePoint(self.bounds.size.width, y)];
        [line setLineWidth:1.0];
        [line stroke];
    }
}

- (void)drawHeader {
    NSDictionary* titleAttributes = @{
        NSFontAttributeName: RoundedFont(42.0, NSFontWeightBold),
        NSForegroundColorAttributeName: [NSColor whiteColor]
    };
    NSDictionary* subtitleAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:16.0 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: RGB(187, 203, 228)
    };

    [@"Морской бой" drawAtPoint:NSMakePoint(36.0, 30.0) withAttributes:titleAttributes];
    [@"Расставьте флот, сохраните партию и ведите бой в удобном оконном интерфейсе." drawAtPoint:NSMakePoint(38.0, 82.0)
                                                                                   withAttributes:subtitleAttributes];

    const Match& game = [_controller game];
    NSString* status = ToNSString(game.statusText());
    NSRect pillRect = NSMakeRect(36.0, 112.0, 520.0, 34.0);
    FillRoundedRect(pillRect, 17.0, RGB(255, 255, 255, 0.10));
    StrokeRoundedRect(pillRect, 17.0, RGB(255, 255, 255, 0.16), 1.0);

    NSDictionary* statusAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:13.5 weight:NSFontWeightSemibold],
        NSForegroundColorAttributeName: [NSColor whiteColor]
    };
    [status drawInRect:NSInsetRect(pillRect, 14.0, 8.0) withAttributes:statusAttributes];
}

- (void)drawStartScreen {
    NSDictionary* titleAttributes = @{
        NSFontAttributeName: RoundedFont(58.0, NSFontWeightBold),
        NSForegroundColorAttributeName: [NSColor whiteColor]
    };
    NSDictionary* subtitleAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:18.0 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: RGB(194, 211, 235)
    };
    NSDictionary* smallAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:14.0 weight:NSFontWeightSemibold],
        NSForegroundColorAttributeName: RGB(155, 178, 207)
    };

    NSRect titleRect = NSMakeRect(0.0, 150.0, self.bounds.size.width, 70.0);
    NSMutableParagraphStyle* centered = [[NSMutableParagraphStyle alloc] init];
    centered.alignment = NSTextAlignmentCenter;
    NSMutableDictionary* centeredTitle = [titleAttributes mutableCopy];
    centeredTitle[NSParagraphStyleAttributeName] = centered;
    [@"Морской бой" drawInRect:titleRect withAttributes:centeredTitle];

    NSMutableDictionary* centeredSubtitle = [subtitleAttributes mutableCopy];
    centeredSubtitle[NSParagraphStyleAttributeName] = centered;
    [@"Новая партия начинается с ручной расстановки кораблей. Продолжение загрузит последнее сохранение." drawInRect:NSMakeRect(160.0, 230.0, self.bounds.size.width - 320.0, 28.0)
                                                                                                      withAttributes:centeredSubtitle];

    NSMutableDictionary* centeredSmall = [smallAttributes mutableCopy];
    centeredSmall[NSParagraphStyleAttributeName] = centered;
    [@"Совет: если окно ниже экрана, прокрутите интерфейс вниз колесом или трекпадом." drawInRect:NSMakeRect(160.0, 560.0, self.bounds.size.width - 320.0, 24.0)
                                                                                  withAttributes:centeredSmall];

    const std::string& error = [_controller game].lastPersistenceError();
    if (!error.empty()) {
        NSMutableDictionary* errorAttributes = [smallAttributes mutableCopy];
        errorAttributes[NSForegroundColorAttributeName] = RGB(255, 174, 142);
        errorAttributes[NSParagraphStyleAttributeName] = centered;
        [ToNSString(error) drawInRect:NSMakeRect(160.0, 600.0, self.bounds.size.width - 320.0, 24.0)
                       withAttributes:errorAttributes];
    }
}

- (void)drawPanel:(NSRect)rect title:(NSString*)title {
    [NSGraphicsContext saveGraphicsState];
    NSShadow* shadow = [[NSShadow alloc] init];
    shadow.shadowBlurRadius = 24.0;
    shadow.shadowOffset = NSMakeSize(0.0, 10.0);
    shadow.shadowColor = RGB(5, 8, 14, 0.35);
    [shadow set];

    NSBezierPath* clip = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:28.0 yRadius:28.0];
    [clip addClip];
    NSGradient* gradient = [[NSGradient alloc] initWithColors:@[
        RGB(22, 35, 56, 0.95),
        RGB(14, 24, 39, 0.97)
    ]];
    [gradient drawInRect:rect angle:90.0];
    [NSGraphicsContext restoreGraphicsState];

    StrokeRoundedRect(rect, 28.0, RGB(160, 190, 228, 0.16), 1.0);

    NSDictionary* attributes = @{
        NSFontAttributeName: RoundedFont(22.0, NSFontWeightBold),
        NSForegroundColorAttributeName: RGB(239, 245, 255)
    };
    [title drawAtPoint:NSMakePoint(rect.origin.x + 30.0, rect.origin.y + 28.0) withAttributes:attributes];
}

- (void)drawBoardAtLayout:(const BoardLayout&)layout
                    title:(NSString*)title
                 showOwn:(BOOL)showOwn
             hoveredCell:(std::optional<BPoint>)hovered
                 lastShot:(std::optional<BPoint>)lastShot {
    [self drawPanel:layout.panelRect title:title];

    NSDictionary* axisAttributes = @{
        NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:13.0 weight:NSFontWeightSemibold],
        NSForegroundColorAttributeName: RGB(174, 194, 221)
    };

    for (int col = 0; col < Board::kSize; ++col) {
        NSString* label = [NSString stringWithFormat:@"%d", col + 1];
        NSRect labelRect = NSMakeRect(RectForCell(layout, {0, col}).origin.x,
                                      layout.gridRect.origin.y - 28.0,
                                      layout.cellSize,
                                      18.0);
        [label drawInRect:labelRect withAttributes:axisAttributes];
    }

    for (int row = 0; row < Board::kSize; ++row) {
        NSString* label = [NSString stringWithFormat:@"%c", 'A' + row];
        NSRect labelRect = NSMakeRect(layout.gridRect.origin.x - 28.0,
                                      RectForCell(layout, {row, 0}).origin.y + 8.0,
                                      18.0,
                                      18.0);
        [label drawInRect:labelRect withAttributes:axisAttributes];
    }

    const Match& game = [_controller game];
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col) {
            BPoint point{row, col};
            NSRect rect = RectForCell(layout, point);
            Cell cell = showOwn ? game.playerCellAt(point) : game.enemyRadarCellAt(point);

            NSColor* fillColor = RGB(44, 92, 140);
            NSColor* strokeColor = RGB(124, 166, 212, 0.20);
            NSString* symbol = @"";
            NSDictionary* symbolAttributes = @{
                NSFontAttributeName: [NSFont systemFontOfSize:20.0 weight:NSFontWeightBold],
                NSForegroundColorAttributeName: [NSColor whiteColor]
            };

            switch (cell) {
                case Cell::Empty:
                    fillColor = RGB(41, 96, 148);
                    break;
                case Cell::Ship:
                    fillColor = RGB(163, 225, 226);
                    strokeColor = RGB(236, 255, 255, 0.55);
                    break;
                case Cell::Miss:
                    fillColor = RGB(72, 95, 126);
                    symbol = @"•";
                    break;
                case Cell::Hit:
                    fillColor = RGB(239, 131, 80);
                    symbol = @"✕";
                    break;
                case Cell::Sunk:
                    fillColor = RGB(220, 71, 71);
                    symbol = @"✕";
                    break;
            }

            if (hovered.has_value() && hovered.value() == point && cell == Cell::Empty) {
                fillColor = RGB(70, 136, 194);
            }

            FillRoundedRect(rect, 10.0, fillColor);
            StrokeRoundedRect(rect, 10.0, strokeColor, 1.0);

            if (lastShot.has_value() && lastShot.value() == point) {
                StrokeRoundedRect(NSInsetRect(rect, -3.0, -3.0), 12.0, RGB(255, 255, 255, 0.70), 2.0);
            }

            if (symbol.length > 0) {
                NSRect symbolRect = NSInsetRect(rect, 0.0, 4.0);
                [symbol drawInRect:symbolRect withAttributes:symbolAttributes];
            }
        }
    }
}

- (void)drawLegendInRect:(NSRect)rect {
    NSDictionary* titleAttributes = @{
        NSFontAttributeName: RoundedFont(18.0, NSFontWeightBold),
        NSForegroundColorAttributeName: RGB(240, 246, 255)
    };
    NSDictionary* textAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:14.0 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: RGB(191, 207, 229)
    };

    [@"Легенда" drawAtPoint:NSMakePoint(rect.origin.x, rect.origin.y) withAttributes:titleAttributes];

    NSArray<NSDictionary*>* items = @[
        @{@"color": RGB(163, 225, 226), @"text": @"ваши корабли"},
        @{@"color": RGB(72, 95, 126), @"text": @"промах"},
        @{@"color": RGB(239, 131, 80), @"text": @"попадание"},
        @{@"color": RGB(220, 71, 71), @"text": @"потопленный корабль"}
    ];

    CGFloat y = rect.origin.y + 40.0;
    for (NSDictionary* item in items) {
        FillRoundedRect(NSMakeRect(rect.origin.x, y, 20.0, 20.0), 6.0, item[@"color"]);
        [item[@"text"] drawAtPoint:NSMakePoint(rect.origin.x + 32.0, y - 1.0) withAttributes:textAttributes];
        y += 34.0;
    }
}

- (void)drawLogInRect:(NSRect)rect {
    NSDictionary* titleAttributes = @{
        NSFontAttributeName: RoundedFont(18.0, NSFontWeightBold),
        NSForegroundColorAttributeName: RGB(240, 246, 255)
    };
    NSDictionary* textAttributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:14.0 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: RGB(191, 207, 229)
    };

    [@"Боевой журнал" drawAtPoint:NSMakePoint(rect.origin.x, rect.origin.y) withAttributes:titleAttributes];

    const std::vector<std::string>& messages = [_controller game].messages();
    CGFloat y = rect.origin.y + 38.0;
    for (const std::string& message : messages) {
        NSString* line = ToNSString(message);
        [line drawInRect:NSMakeRect(rect.origin.x, y, rect.size.width, 20.0) withAttributes:textAttributes];
        y += 24.0;
    }
}

- (void)drawBottomPanel {
    NSRect rect = NSMakeRect(36.0, NSMaxY(MakeBoardLayout(self.bounds, true).panelRect) + 28.0,
                             self.bounds.size.width - 72.0,
                             self.bounds.size.height - NSMaxY(MakeBoardLayout(self.bounds, true).panelRect) - 64.0);
    [self drawPanel:rect title:ToNSString([_controller game].titleText())];

    [self drawLegendInRect:NSMakeRect(rect.origin.x + 30.0, rect.origin.y + 34.0, 280.0, rect.size.height - 60.0)];
    [self drawLogInRect:NSMakeRect(rect.origin.x + 330.0, rect.origin.y + 34.0,
                                   rect.size.width - 360.0, rect.size.height - 60.0)];

    if ([_controller isPlacingFleet]) {
        NSDictionary* setupAttributes = @{
            NSFontAttributeName: [NSFont systemFontOfSize:14.0 weight:NSFontWeightSemibold],
            NSForegroundColorAttributeName: RGB(240, 246, 255)
        };
        NSString* setupText = [NSString stringWithFormat:@"Следующий корабль: %d палубы. Направление: %@.",
                                                         [_controller nextShipLength],
                                                         [_controller isHorizontalPlacement] ? @"горизонтально" : @"вертикально"];
        [setupText drawAtPoint:NSMakePoint(rect.origin.x + 30.0, NSMaxY(rect) - 34.0) withAttributes:setupAttributes];
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    [self drawBackgroundInRect:dirtyRect];

    if ([_controller isMainMenu]) {
        [self drawStartScreen];
        return;
    }

    [self drawHeader];

    BoardLayout playerLayout = MakeBoardLayout(self.bounds, true);
    BoardLayout enemyLayout = MakeBoardLayout(self.bounds, false);
    const Match& game = [_controller game];

    [self drawBoardAtLayout:playerLayout
                      title:@"Ваш флот"
                   showOwn:YES
               hoveredCell:_hoveredPlayerCell
                   lastShot:game.lastEnemyShot()];

    [self drawBoardAtLayout:enemyLayout
                      title:@"Радар противника"
                   showOwn:NO
               hoveredCell:_hoveredEnemyCell
                   lastShot:game.lastPlayerShot()];

    [self drawBottomPanel];
}

@end

@implementation BattleshipController {
    NSWindow* _window;
    BattleshipView* _view;
    std::unique_ptr<Match> _game;
    AppScreen _screen;
    BOOL _horizontalPlacement;
    BOOL _aiThinking;
}

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _game = std::make_unique<Match>(battleship::infrastructure::createDefaultMatch());
    _screen = AppScreen::MainMenu;
    _horizontalPlacement = YES;
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0.0, 0.0, 1180.0, 760.0);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled |
                                                    NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskMiniaturizable |
                                                    NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"Морской бой";
    _window.minSize = NSMakeSize(920.0, 620.0);

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:frame];
    scrollView.hasVerticalScroller = YES;
    scrollView.hasHorizontalScroller = NO;
    scrollView.autohidesScrollers = YES;
    scrollView.borderType = NSNoBorder;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    NSRect contentFrame = NSMakeRect(0.0, 0.0, 1180.0, 940.0);
    _view = [[BattleshipView alloc] initWithFrame:contentFrame controller:self];
    _view.autoresizingMask = NSViewWidthSizable;
    scrollView.documentView = _view;
    _window.contentView = scrollView;
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

- (const Match&)game {
    return *_game;
}

- (BOOL)isMainMenu {
    return _screen == AppScreen::MainMenu;
}

- (BOOL)canInteract {
    return _screen == AppScreen::Game && !_aiThinking && _game->turn() == Turn::Player && !_game->isFinished() &&
           _game->phase() == battleship::domain::Phase::Battle;
}

- (BOOL)canShuffle {
    return _screen == AppScreen::Game && !_aiThinking && _game->canReshuffle();
}

- (BOOL)canStartBattle {
    return _screen == AppScreen::Game && !_aiThinking && _game->canStartBattle();
}

- (BOOL)isPlacingFleet {
    return _screen == AppScreen::Game && !_aiThinking && _game->isPlacingFleet();
}

- (BOOL)isHorizontalPlacement {
    return _horizontalPlacement;
}

- (int)nextShipLength {
    return _game->nextShipLength();
}

- (void)refresh {
    [_view refreshControls];
    [_view setNeedsLayout:YES];
    [_view setNeedsDisplay:YES];
}

- (void)queueEnemyTurnIfNeeded {
    if (_aiThinking || _game->turn() != Turn::Enemy || _game->isFinished()) {
        return;
    }

    _aiThinking = YES;
    [self refresh];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.55 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        self->_aiThinking = NO;
        self->_game->enemyShoot();
        [self refresh];
        [self queueEnemyTurnIfNeeded];
    });
}

- (void)handleEnemyCellClick:(BPoint)point {
    _game->playerShoot(point);
    [self refresh];
    [self queueEnemyTurnIfNeeded];
}

- (void)handlePlayerCellClick:(BPoint)point {
    _game->placePlayerShip(point, _horizontalPlacement);
    [self refresh];
}

- (void)startNewGame:(id)sender {
    (void)sender;
    _screen = AppScreen::Game;
    _aiThinking = NO;
    _horizontalPlacement = YES;
    _game->reset();
    [self refresh];
}

- (void)shuffleFleet:(id)sender {
    (void)sender;
    _game->reshufflePlayerFleet();
    [self refresh];
}

- (void)clearFleet:(id)sender {
    (void)sender;
    _game->clearPlayerFleet();
    [self refresh];
}

- (void)rotatePlacement:(id)sender {
    (void)sender;
    _horizontalPlacement = !_horizontalPlacement;
    [self refresh];
}

- (void)startBattle:(id)sender {
    (void)sender;
    _game->startBattle();
    [self refresh];
}

- (void)showMainMenu:(id)sender {
    (void)sender;
    _screen = AppScreen::MainMenu;
    _aiThinking = NO;
    [self refresh];
}

- (void)saveGame:(id)sender {
    (void)sender;
    _game->save();
    [self refresh];
}

- (void)loadGame:(id)sender {
    (void)sender;
    _aiThinking = NO;
    if (_game->load()) {
        _screen = AppScreen::Game;
    }
    [self refresh];
}

- (void)exitGame:(id)sender {
    (void)sender;
    [NSApp terminate:nil];
}

@end

int main(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;

    @autoreleasepool {
        [NSApplication sharedApplication];

        NSMenu* menuBar = [[NSMenu alloc] init];
        NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
        [menuBar addItem:appMenuItem];
        [NSApp setMainMenu:menuBar];

        NSMenu* appMenu = [[NSMenu alloc] init];
        NSString* appName = @"Морской бой";
        NSString* quitTitle = [@"Закрыть " stringByAppendingString:appName];
        NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
        [appMenu addItem:quitItem];
        [appMenuItem setSubmenu:appMenu];

        BattleshipController* delegate = [[BattleshipController alloc] init];
        [NSApp setDelegate:delegate];
        [NSApp run];
    }

    return 0;
}
