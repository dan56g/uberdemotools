using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class ChatRule
    {
        public string Operator = "Contains";
        public string Value = "";
        public bool CaseSensitive = false;
        public bool IgnoreColors = true;
    }

    public class UdtConfig
    {
        public List<ChatRule> ChatRules = new List<ChatRule>();
        public int ChatCutStartOffset = 10;
        public int ChatCutEndOffset = 10;
        public bool OutputToInputFolder = true;
        public string OutputFolder = "";
        public bool SkipChatOffsetsDialog = false;
        public bool SkipScanFoldersRecursivelyDialog = false;
        public bool ScanFoldersRecursively = false;
        public int MaxThreadCount = 4;
    }

    public class DemoInfo
    {
        public string FilePath;
        public string Protocol;
        public List<ChatEventDisplayInfo> ChatEvents = new List<ChatEventDisplayInfo>();
        // @TODO:
    }

    public class App
    {
        private const string GuiVersion = "0.3.0";
        private readonly string DllVersion = UDT_DLL.GetVersion();

        private static readonly List<string> DemoExtensions = new List<string>
        {
            ".dm_68",
            ".dm_73",
            ".dm_90"
        };

        private static readonly Dictionary<string, UDT_DLL.udtProtocol> ProtocolFileExtDic = new Dictionary<string, UDT_DLL.udtProtocol>
        {
            { ".dm_68", UDT_DLL.udtProtocol.Dm68 },
            { ".dm_73", UDT_DLL.udtProtocol.Dm73 },
            { ".dm_90", UDT_DLL.udtProtocol.Dm90 }
        };

        private class ConfigStringDisplayInfo
        {
            public ConfigStringDisplayInfo(string description, string value)
            {
                Description = description;
                Value = value;
            }

            public string Description { get; set; }
            public string Value { get; set; }
        }

        private class DemoDisplayInfo
        {
            public string FileName { get; set; }
            public DemoInfo Demo { get; set; }
        }

        private UdtConfig _config = new UdtConfig();
        private Application _application = null;
        private Thread _jobThread = null;
        private Window _window = null;
        private ListView _demoListView = null;
        private Brush _demoListViewBackground = null;
        private ListView _infoListView = null;
        private ListBox _logListBox = null;
        private ProgressBar _progressBar = null;
        private Button _cancelJobButton = null;
        private GroupBox _progressGroupBox = null;
        private DockPanel _rootPanel = null;
        private TabControl _tabControl = null;
        private List<FrameworkElement> _rootElements = new List<FrameworkElement>();
        private List<DemoInfo> _demos = new List<DemoInfo>();
        private AlternatingListBoxBackground _altListBoxBg = null;
        private UDT_DLL.udtParseArg _parseArg = new UDT_DLL.udtParseArg();
        private List<AppComponent> _appComponents = new List<AppComponent>();
        private static RoutedCommand _cutByChatCommand = new RoutedCommand();
        private static RoutedCommand _deleteDemoCommand = new RoutedCommand();
        private static RoutedCommand _showDemoInfoCommand = new RoutedCommand();
        private static RoutedCommand _clearLogCommand = new RoutedCommand();
        private static RoutedCommand _copyLogCommand = new RoutedCommand();
        private static RoutedCommand _copyChatCommand = new RoutedCommand();
        private static RoutedCommand _copyFragCommand = new RoutedCommand();

        public UdtConfig Config
        {
            get { return _config; }
        }

        public App(string[] cmdLineArgs)
        {
            PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Critical;

            UDT_DLL.SetFatalErrorHandler(FatalErrorHandler);

            LoadConfig();

            _altListBoxBg = new AlternatingListBoxBackground(Colors.White, Color.FromRgb(223, 223, 223));

            var manageDemosTab = new TabItem();
            manageDemosTab.Header = "Manage Demos";
            manageDemosTab.Content = CreateManageDemosTab();

            var demosTab = new TabItem();
            demosTab.Header = "Info";
            demosTab.Content = CreateDemoInfoTab();

            var chatEvents = new ChatEventsComponent(this);
            _appComponents.Add(chatEvents);
            var demoChatTab = new TabItem();
            demoChatTab.Header = "Chat";
            demoChatTab.Content = chatEvents.RootControl;
            
            var cutTimeTab = new TabItem();
            cutTimeTab.Header = "Cut by Time";
            cutTimeTab.Content = new Label(); // @TODO:

            var cutChatTab = new TabItem();
            cutChatTab.Header = "Cut by Chat";
            cutChatTab.Content = new Label(); // @TODO:

            var settings = new AppSettingsComponent(this);
            _appComponents.Add(settings);
            var settingsTab = new TabItem();
            settingsTab.Header = "Settings";
            settingsTab.Content = settings.RootControl;
            
            var tabControl = new TabControl();
            _tabControl = tabControl;
            tabControl.HorizontalAlignment = HorizontalAlignment.Stretch;
            tabControl.VerticalAlignment = VerticalAlignment.Stretch;
            tabControl.Margin = new Thickness(5);
            tabControl.Items.Add(manageDemosTab);
            tabControl.Items.Add(demosTab);
            tabControl.Items.Add(demoChatTab);
            tabControl.Items.Add(cutTimeTab);
            tabControl.Items.Add(cutChatTab);
            tabControl.Items.Add(settingsTab);
            tabControl.SelectionChanged += (obj, args) => OnTabSelectionChanged();

            var exitMenuItem = new MenuItem();
            exitMenuItem.Header = "E_xit";
            exitMenuItem.Click += (obj, arg) => OnQuit();
            exitMenuItem.ToolTip = new ToolTip { Content = "Close the program" };

            var openDemoMenuItem = new MenuItem();
            openDemoMenuItem.Header = "Open Demo(s)...";
            openDemoMenuItem.Click += (obj, arg) => OnOpenDemo();
            openDemoMenuItem.ToolTip = new ToolTip { Content = "Open a demo file for processing/analysis" };

            var openFolderMenuItem = new MenuItem();
            openFolderMenuItem.Header = "Open Demo Folder...";
            openFolderMenuItem.Click += (obj, arg) => OnOpenDemoFolder();
            openFolderMenuItem.ToolTip = new ToolTip { Content = "Open a demo folder for processing/analysis" };

            var fileMenuItem = new MenuItem();
            fileMenuItem.Header = "_File";
            fileMenuItem.Items.Add(openDemoMenuItem);
            fileMenuItem.Items.Add(openFolderMenuItem);
            fileMenuItem.Items.Add(new Separator());
            fileMenuItem.Items.Add(exitMenuItem);

            var clearLogMenuItem = new MenuItem();
            clearLogMenuItem.Header = "Clear";
            clearLogMenuItem.Click += (obj, arg) => ClearLog();
            clearLogMenuItem.ToolTip = new ToolTip { Content = "Clears the log box" };

            var copyLogMenuItem = new MenuItem();
            copyLogMenuItem.Header = "Copy Everything";
            copyLogMenuItem.Click += (obj, arg) => CopyLog();
            copyLogMenuItem.ToolTip = new ToolTip { Content = "Copies the log to the Windows clipboard" };

            var copyLogSelectionMenuItem = new MenuItem();
            copyLogSelectionMenuItem.Header = "Copy Selection";
            copyLogSelectionMenuItem.Click += (obj, arg) => CopyLogSelection();
            copyLogSelectionMenuItem.ToolTip = new ToolTip { Content = "Copies the selected log line to the Windows clipboard" };

            var saveLogMenuItem = new MenuItem();
            saveLogMenuItem.Header = "Save to File...";
            saveLogMenuItem.Click += (obj, arg) => SaveLog();
            saveLogMenuItem.ToolTip = new ToolTip { Content = "Saves the log to the specified file" };

            var logMenuItem = new MenuItem();
            logMenuItem.Header = "_Log";
            logMenuItem.Items.Add(clearLogMenuItem);
            logMenuItem.Items.Add(copyLogMenuItem);
            logMenuItem.Items.Add(copyLogSelectionMenuItem);
            logMenuItem.Items.Add(new Separator());
            logMenuItem.Items.Add(saveLogMenuItem);

            var aboutMenuItem = new MenuItem();
            aboutMenuItem.Header = "_About";
            aboutMenuItem.Click += (obj, arg) => ShowAboutWindow();
            aboutMenuItem.ToolTip = new ToolTip { Content = "Learn more about this awesome application" };

            var helpMenuItem = new MenuItem();
            helpMenuItem.Header = "_Help";
            helpMenuItem.Items.Add(aboutMenuItem);

            var mainMenu = new Menu();
            mainMenu.IsMainMenu = true;
            mainMenu.Items.Add(fileMenuItem);
            mainMenu.Items.Add(logMenuItem);
            mainMenu.Items.Add(helpMenuItem);

            var logListBox = new ListBox();
            _logListBox = logListBox;
            _logListBox.SelectionMode = SelectionMode.Single;
            logListBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logListBox.VerticalAlignment = VerticalAlignment.Stretch;
            logListBox.Margin = new Thickness(5);
            logListBox.Height = 150;
            InitLogListBoxClearCommand();
            InitLogListBoxCopyCommand();
            InitLogListBoxContextualMenu();

            var logGroupBox = new GroupBox();
            logGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            logGroupBox.Margin = new Thickness(5);
            logGroupBox.Header = "Log";
            logGroupBox.Content = logListBox;

            var progressBar = new ProgressBar();
            _progressBar = progressBar;
            progressBar.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressBar.VerticalAlignment = VerticalAlignment.Bottom;
            progressBar.Margin = new Thickness(5);
            progressBar.Height = 20;
            progressBar.Minimum = 0;
            progressBar.Maximum = 100;
            progressBar.Value = 0;

            var cancelJobButton = new Button();
            _cancelJobButton = cancelJobButton;
            cancelJobButton.HorizontalAlignment = HorizontalAlignment.Right;
            cancelJobButton.VerticalAlignment = VerticalAlignment.Center;
            cancelJobButton.Width = 70;
            cancelJobButton.Height = 25;
            cancelJobButton.Content = "Cancel";
            cancelJobButton.Click += (obj, args) => OnCancelJobClicked();

            var progressPanel = new DockPanel();
            progressPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressPanel.VerticalAlignment = VerticalAlignment.Bottom;
            progressPanel.LastChildFill = true;
            progressPanel.Children.Add(cancelJobButton);
            progressPanel.Children.Add(progressBar);
            DockPanel.SetDock(cancelJobButton, Dock.Right);

            var progressGroupBox = new GroupBox();
            _progressGroupBox = progressGroupBox;
            progressGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressGroupBox.VerticalAlignment = VerticalAlignment.Center;
            progressGroupBox.Margin = new Thickness(5, 0, 5, 0);
            progressGroupBox.Header = "Progress";
            progressGroupBox.Content = progressPanel;
            progressGroupBox.Visibility = Visibility.Collapsed;

            var demoGridView = new GridView();
            demoGridView.AllowsColumnReorder = false;
            demoGridView.Columns.Add(new GridViewColumn { Header = "File Name", Width = 290, DisplayMemberBinding = new Binding("FileName") });

            var demoListView = new ListView();
            _demoListView = demoListView;
            _demoListViewBackground = demoListView.Background;
            demoListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            demoListView.VerticalAlignment = VerticalAlignment.Stretch;
            demoListView.Margin = new Thickness(5);
            demoListView.Width = 300;
            demoListView.AllowDrop = true;
            demoListView.View = demoGridView;
            demoListView.SelectionMode = SelectionMode.Extended;
            demoListView.DragEnter += OnDemoListBoxDragEnter;
            demoListView.Drop += OnDemoListBoxDragDrop;
            demoListView.SelectionChanged += (obj, args) => OnDemoListSelectionChanged();
            demoListView.Initialized += (obj, arg) => { _demoListViewBackground = _demoListView.Background; };
            demoListView.Foreground = new SolidColorBrush(Colors.Black);
            InitDemoListDeleteCommand();
            
            var demoListGroupBox = new GroupBox();
            demoListGroupBox.Header = "Demo List";
            demoListGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            demoListGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            demoListGroupBox.Margin = new Thickness(5);
            demoListGroupBox.Content = demoListView;

            var centerPartPanel = new DockPanel();
            centerPartPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            centerPartPanel.VerticalAlignment = VerticalAlignment.Stretch;
            centerPartPanel.Margin = new Thickness(5);
            centerPartPanel.Children.Add(demoListGroupBox);
            centerPartPanel.Children.Add(tabControl);
            centerPartPanel.LastChildFill = true;
            DockPanel.SetDock(demoListGroupBox, Dock.Left);

            var statusBarTextBox = new TextBox();
            statusBarTextBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            statusBarTextBox.VerticalAlignment = VerticalAlignment.Bottom;
            statusBarTextBox.IsEnabled = true;
            statusBarTextBox.IsReadOnly = true;
            statusBarTextBox.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            statusBarTextBox.Text = Quotes.GetRandomQuote();

            var rootPanel = new DockPanel();
            _rootPanel = rootPanel;
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.LastChildFill = true;
            AddRootElement(statusBarTextBox);
            AddRootElement(logGroupBox);
            AddRootElement(progressGroupBox);
            AddRootElement(mainMenu);
            AddRootElement(centerPartPanel);
            DockPanel.SetDock(mainMenu, Dock.Top);
            DockPanel.SetDock(centerPartPanel, Dock.Top);
            DockPanel.SetDock(statusBarTextBox, Dock.Bottom);
            DockPanel.SetDock(logGroupBox, Dock.Bottom);
            DockPanel.SetDock(progressGroupBox, Dock.Bottom);
            _rootElements.Remove(progressGroupBox); // Only the cancel button can remain active at all times.

            _altListBoxBg.ApplyTo(_infoListView);
            _altListBoxBg.ApplyTo(_demoListView);
            _altListBoxBg.ApplyTo(_logListBox);
            foreach(var component in _appComponents)
            {
                var listViews = component.ListViews;
                if(listViews == null)
                {
                    continue;
                }

                foreach(var listView in listViews)
                {
                    _altListBoxBg.ApplyTo(listView);
                }
            }

            _logListBox.Resources.Add(SystemColors.HighlightBrushKey, new SolidColorBrush(Color.FromRgb(255, 255, 191)));

            var label = new Label { Content = "You can drag'n'drop files and folders here.", HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
            var brush = new VisualBrush(label) { Stretch = Stretch.None, Opacity = 0.5 };
            _demoListView.Background = brush;

            var window = new Window();

            TextOptions.SetTextRenderingMode(window, TextRenderingMode.ClearType);
            TextOptions.SetTextHintingMode(window, TextHintingMode.Fixed);
            TextOptions.SetTextFormattingMode(window, TextFormattingMode.Display);

            _window = window;
            window.Closing += (obj, args) => OnQuit();
            window.WindowStyle = WindowStyle.SingleBorderWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = true;
            window.Title = "UDT";
            window.Content = rootPanel;
            window.Width = 1024;
            window.Height = 768;
            window.MinWidth = 1024;
            window.MinHeight = 768;

#if UDT_X64
            const string arch = "x64";
#else
            const string arch = "x86";
#endif
            LogInfo("UDT {0} is now operational!", arch);
            LogInfo("GUI version: " + GuiVersion);
            LogInfo("DLL version: " + DllVersion);

            ProcessCommandLine(cmdLineArgs);

            var app = new Application();
            _application = app;
            app.ShutdownMode = ShutdownMode.OnLastWindowClose;
            app.Run(window);
        }

        private void ProcessCommandLine(string[] cmdLineArgs)
        {
            var filePaths = new List<string>();
            var folderPaths = new List<string>();
            foreach(var argument in cmdLineArgs)
            {
                var arg = argument;
                if(Path.GetExtension(arg).ToLower() == ".lnk")
                {
                    string realPath;
                    if(Shortcut.ResolveShortcut(out realPath, argument))
                    {
                        arg = realPath;
                    }
                }

                if(File.Exists(arg) && IsDemoPath(arg))
                {
                    filePaths.Add(Path.GetFullPath(arg));
                }
                else if(Directory.Exists(arg))
                {
                    folderPaths.Add(Path.GetFullPath(arg));
                }
            }

            AddDemos(filePaths, folderPaths);
        }

        private void AddDemos(List<string> filePaths, List<string> folderPaths)
        {
            var filteredFilePaths = new List<string>();
            foreach(var filePath in filePaths)
            {
                if(!File.Exists(filePath))
                {
                    continue;
                }

                var absFilePath = Path.GetFullPath(filePath).ToLower();

                bool exists = false;
                foreach(var demo in _demos)
                {
                    var demoAbsFilePath = Path.GetFullPath(demo.FilePath).ToLower();
                    if(demoAbsFilePath == absFilePath)
                    {
                        exists = true;
                        break;
                    }
                }

                if(!exists)
                {
                    filteredFilePaths.Add(filePath);
                }
            }

            if(folderPaths.Count > 0)
            {
                var recursive = false;

                if(_config.SkipScanFoldersRecursivelyDialog)
                {
                    recursive = _config.ScanFoldersRecursively;
                }
                else
                {
                    var result = MessageBox.Show("You dropped folder paths. Do you want to add all demos in subdirectories too?", "Scan for Demos Recursively?", MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
                    if(result == MessageBoxResult.Cancel)
                    {
                        return;
                    }

                    recursive = result == MessageBoxResult.Yes;
                }

                var searchOption = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;

                foreach(var folderPath in folderPaths)
                {
                    foreach(var demoExtension in DemoExtensions)
                    {
                        var searchPattern = "*" + demoExtension;
                        var demoPaths = Directory.GetFiles(folderPath, searchPattern, searchOption);
                        filteredFilePaths.AddRange(demoPaths);
                    }
                }
            }

            if(filteredFilePaths.Count == 0)
            {
                return;
            }

            AddDemosImpl(filteredFilePaths);
        }

        private void InitDemoListDeleteCommand()
        {
            var inputGesture = new KeyGesture(Key.Delete, ModifierKeys.None);
            var inputBinding = new KeyBinding(_deleteDemoCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _deleteDemoCommand;
            commandBinding.Executed += (obj, args) => OnRemoveDemoClicked();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _demoListView.InputBindings.Add(inputBinding);
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxClearCommand()
        {
            var inputGesture = new KeyGesture(Key.X, ModifierKeys.Control);
            var inputBinding = new KeyBinding(_clearLogCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _clearLogCommand;
            commandBinding.Executed += (obj, args) => ClearLog();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _logListBox.InputBindings.Add(inputBinding);
            _logListBox.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxCopyCommand()
        {
            var inputGesture = new KeyGesture(Key.C, ModifierKeys.Control);
            var inputBinding = new KeyBinding(_copyLogCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _copyLogCommand;
            commandBinding.Executed += (obj, args) => CopyLogSelection();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _logListBox.InputBindings.Add(inputBinding);
            _logListBox.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxContextualMenu()
        {
            var clearLogMenuItem = new MenuItem();
            clearLogMenuItem.Header = "Clear (Ctrl-X)";
            clearLogMenuItem.Command = _clearLogCommand;
            clearLogMenuItem.Click += (obj, args) => ClearLog();

            var copyLogMenuItem = new MenuItem();
            copyLogMenuItem.Header = "Copy (Ctrl-C)";
            copyLogMenuItem.Command = _copyLogCommand;
            copyLogMenuItem.Click += (obj, args) => CopyLogSelection();

            var contextMenu = new ContextMenu();
            contextMenu.Items.Add(clearLogMenuItem);
            contextMenu.Items.Add(copyLogMenuItem);

            _logListBox.ContextMenu = contextMenu;
        }

        private void OnShowDemoInfo()
        {
            VoidDelegate tabSetter = delegate
            {
                _tabControl.SelectedIndex = 1;
            };
            _tabControl.Dispatcher.Invoke(tabSetter);
        }

        private void AddRootElement(FrameworkElement element)
        {
            _rootPanel.Children.Add(element);
            _rootElements.Add(element);
        }

        private void OnQuit()
        {
            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            SaveConfig();
            _application.Shutdown();
        }

        private void OnTabSelectionChanged()
        {
            var singleMode = (_tabControl.SelectedIndex == 1) || (_tabControl.SelectedIndex == 2);
            _demoListView.SelectionMode = singleMode ? SelectionMode.Single : SelectionMode.Extended;
        }

        private void OnOpenDemo()
        {
            using(var openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                // @TODO: Construct the filter programmatically.
                openFileDialog.CheckPathExists = true;
                openFileDialog.Multiselect = true;
                openFileDialog.InitialDirectory = System.Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                openFileDialog.Filter = "Quake 3 demos (*.dm_68)|*.dm_68|Quake Live demos (*.dm_73;*.dm_90)|*.dm_73;*.dm_90";
                if(openFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                var filePaths = new List<string>();
                filePaths.AddRange(openFileDialog.FileNames);
                AddDemos(filePaths, new List<string>());
            }
        }

        private void OnOpenDemoFolder()
        {
            using(var openFolderDialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                openFolderDialog.Description = "Browse for a folder containing demo files";
                openFolderDialog.ShowNewFolderButton = true;
                openFolderDialog.RootFolder = Environment.SpecialFolder.Desktop;
                if(openFolderDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                AddDemos(new List<string>(), new List<string> { openFolderDialog.SelectedPath });
            }
        }

        private void ShowAboutWindow()
        {
            var textPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            textPanelList.Add(CreateTuple("Version", GuiVersion));
            textPanelList.Add(CreateTuple("Developer", "myT"));
            var textPanel = WpfHelper.CreateDualColumnPanel(textPanelList, 100, 1);

            var image = new System.Windows.Controls.Image();
            image.HorizontalAlignment = HorizontalAlignment.Right;
            image.VerticalAlignment = VerticalAlignment.Top;
            image.Margin = new Thickness(5);
            image.Stretch = Stretch.None;
            image.Source = UDT.Properties.Resources.UDTIcon.ToImageSource();

            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(textPanel);
            rootPanel.Children.Add(image);

            var window = new Window();
            window.WindowStyle = WindowStyle.ToolWindow;
            window.ResizeMode = ResizeMode.NoResize;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Title = "About UberDemoTools";
            window.Content = rootPanel;
            window.Width = 240;
            window.Height = 100;
            window.Left = _window.Left + (_window.Width - window.Width) / 2;
            window.Top = _window.Top + (_window.Height - window.Height) / 2;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.ShowDialog();
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, FrameworkElement b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, b);
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, string b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, new Label { Content = b });
        }

        private FrameworkElement CreateDemoInfoTab()
        {
            var configStringGridView = new GridView();
            configStringGridView.AllowsColumnReorder = false;
            configStringGridView.Columns.Add(new GridViewColumn { Header = "Name", Width = 150, DisplayMemberBinding = new Binding("Description") });
            configStringGridView.Columns.Add(new GridViewColumn { Header = "Value", Width = 400, DisplayMemberBinding = new Binding("Value") });

            var infoListView = new ListView();
            _infoListView = infoListView;
            infoListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoListView.VerticalAlignment = VerticalAlignment.Stretch;
            infoListView.Margin = new Thickness(5);
            infoListView.View = configStringGridView;
            infoListView.SelectionMode = SelectionMode.Single;
            infoListView.Foreground = new SolidColorBrush(Colors.Black);

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Demo Information";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = infoListView;

            return infoPanelGroupBox;
        }

        private FrameworkElement CreateManageDemosTab()
        {
            var addButton = new Button();
            addButton.Content = "Add File(s)";
            addButton.Width = 75;
            addButton.Height = 25;
            addButton.Margin = new Thickness(5);
            addButton.Click += (obj, args) => OnOpenDemo();

            var addFolderButton = new Button();
            addFolderButton.Content = "Add Folder";
            addFolderButton.Width = 75;
            addFolderButton.Height = 25;
            addFolderButton.Margin = new Thickness(5);
            addFolderButton.Click += (obj, args) => OnOpenDemoFolder();

            var removeButton = new Button();
            removeButton.Content = "Remove";
            removeButton.Width = 75;
            removeButton.Height = 25;
            removeButton.Margin = new Thickness(5);
            removeButton.Click += (obj, args) => OnRemoveDemoClicked();

            var buttonPanel = new StackPanel();
            buttonPanel.HorizontalAlignment = HorizontalAlignment.Left;
            buttonPanel.VerticalAlignment = VerticalAlignment.Top;
            buttonPanel.Margin = new Thickness(5);
            buttonPanel.Orientation = Orientation.Vertical;
            buttonPanel.Children.Add(addButton);
            buttonPanel.Children.Add(addFolderButton);
            buttonPanel.Children.Add(removeButton);

            var buttonGroupBox = new GroupBox();
            buttonGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            buttonGroupBox.VerticalAlignment = VerticalAlignment.Top;
            buttonGroupBox.Margin = new Thickness(5);
            buttonGroupBox.Header = "Demo List Actions";
            buttonGroupBox.Content = buttonPanel;

            return buttonGroupBox;
        }

        private void PopulateInfoListView(DemoInfo demoInfo)
        {
            _infoListView.Items.Clear();
            _infoListView.Items.Add(new ConfigStringDisplayInfo("Folder Path", Path.GetDirectoryName(demoInfo.FilePath) ?? "N/A"));
            _infoListView.Items.Add(new ConfigStringDisplayInfo("File Name", Path.GetFileNameWithoutExtension(demoInfo.FilePath) ?? "N/A"));
            // @TODO:
        }

        private void OnDemoListSelectionChanged()
        {
            int idx = _demoListView.SelectedIndex;
            if(idx < 0 || idx >= _demos.Count)
            {
                return;
            }

            var demoInfo = _demos[idx];

            PopulateInfoListView(demoInfo);
            foreach(var tab in _appComponents)
            {
                tab.PopulateViews(demoInfo);
            }
        }

        private void OnDemoListBoxDragEnter(object sender, DragEventArgs e)
        {
            bool dataPresent = e.Data.GetDataPresent(DataFormats.FileDrop);

            e.Effects = dataPresent ? DragDropEffects.All : DragDropEffects.None;
        }

        private static bool IsDemoPath(string path)
        {
            var extension = Path.GetExtension(path).ToLower();

            return DemoExtensions.Contains(extension);
        }

        private void OnDemoListBoxDragDrop(object sender, DragEventArgs e)
        {
            var droppedPaths = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            var droppedFilePaths = new List<string>();
            var droppedFolderPaths = new List<string>();

            foreach(var droppedPath in droppedPaths)
            {
                var path = droppedPath;
                if(Path.GetExtension(path).ToLower() == ".lnk")
                {
                    string realPath;
                    if(Shortcut.ResolveShortcut(out realPath, path))
                    {
                        path = realPath;
                    }
                }

                if(File.Exists(path) && IsDemoPath(path))
                {
                    droppedFilePaths.Add(path);
                }
                else if(Directory.Exists(path))
                {
                    droppedFolderPaths.Add(path);
                }
            }

            AddDemos(droppedFilePaths, droppedFolderPaths);
        }

        private void DisableUiNonThreadSafe()
        {
            _progressGroupBox.Visibility = Visibility.Visible;
            _progressBar.Value = 0;
            foreach(var element in _rootElements)
            {
                element.IsEnabled = false;
            }
        }

        private void EnableUiThreadSafe()
        {
            VoidDelegate guiResetter = delegate
            {
                _progressGroupBox.Visibility = Visibility.Collapsed;
                _progressBar.Value = 0;
                foreach(var element in _rootElements)
                {
                    element.IsEnabled = true;
                }
            };

            _window.Dispatcher.Invoke(guiResetter);
        }

        private void AddDemosImpl(List<string> filePaths)
        {
            if(filePaths.Count == 0)
            {
                return;
            }

            DisableUiNonThreadSafe();
            _demoListView.Background = _demoListViewBackground;

            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            _jobThread = new Thread(DemoAddThread);
            _jobThread.Start(filePaths);
        }

        private void DemoAddThread(object arg)
        {
            try
            {
                DemoAddThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        delegate void VoidDelegate();

        private void AddDemo(DemoDisplayInfo info)
        {
            var inputGesture = new MouseGesture(MouseAction.LeftDoubleClick, ModifierKeys.None);
            var inputBinding = new MouseBinding(_showDemoInfoCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _showDemoInfoCommand;
            commandBinding.Executed += (obj, args) => OnShowDemoInfo();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };

            var removeDemoItem = new MenuItem();
            removeDemoItem.Header = "Remove (del)";
            removeDemoItem.Command = _deleteDemoCommand;
            removeDemoItem.Click += (obj, args) => OnRemoveDemoClicked();

            var demosContextMenu = new ContextMenu();
            demosContextMenu.Items.Add(removeDemoItem);

            var item = new ListViewItem();
            item.Content = info;
            item.ContextMenu = demosContextMenu;
            item.InputBindings.Add(inputBinding);
            item.CommandBindings.Add(commandBinding);

            _demoListView.Items.Add(item);
        }

        private string GetOutputFolder()
        {
            return _config.OutputToInputFolder ? null : _config.OutputFolder;
        }

        private void DemoAddThreadImpl(object arg)
        {
            var filePaths = (List<string>)arg;

            SetProgressThreadSafe(0.0);

            var outputFolder = GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);

            _parseArg.CancelOperation = 0;
            _parseArg.MessageCb = DemoLoggingCallback;
            _parseArg.OutputFolderPath = outputFolderPtr;
            _parseArg.ProgressCb = DemoProgressCallback;
            _parseArg.ProgressContext = IntPtr.Zero;

            List<DemoInfo> demoInfos = null;
            try
            {
                demoInfos = UDT_DLL.ParseDemos(ref _parseArg, filePaths, _config.MaxThreadCount);
            }
            catch(SEHException exception)
            {
                LogError("Caught an exception while parsing demos: {0}", exception.Message);
                demoInfos = null;
            }

            if(demoInfos == null || demoInfos.Count != filePaths.Count)
            {
                Marshal.FreeHGlobal(outputFolderPtr);
                EnableUiThreadSafe();
                return;
            }

            foreach(var demoInfo in demoInfos)
            {
                var demoDisplayInfo = new DemoDisplayInfo();
                demoDisplayInfo.Demo = demoInfo;
                demoDisplayInfo.FileName = Path.GetFileNameWithoutExtension(demoInfo.FilePath);
                _demos.Add(demoInfo);

                VoidDelegate itemAdder = delegate { AddDemo(demoDisplayInfo); };
                _demoListView.Dispatcher.Invoke(itemAdder);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
            EnableUiThreadSafe();
        }

        private static void RemoveListViewItem<T>(T info, ListView listView) where T : class
        {
            int idx = -1;
            for(int i = 0; i < listView.Items.Count; ++i)
            {
                var listViewItem = listView.Items[i] as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var itemContent = listViewItem.Content as T;
                if(itemContent != null)
                {
                    idx = i;
                    break;
                }
            }

            if(idx != -1)
            {
                listView.Items.RemoveAt(idx);
            }
        }

        private static T FindAnchestor<T>(DependencyObject current) where T : DependencyObject
        {
            do
            {
                if(current is T)
                {
                    return (T)current;
                }
                current = VisualTreeHelper.GetParent(current);
            }
            while(current != null);

            return null;
        }

        private void LoadConfig()
        {
            Serializer.FromXml("Config.xml", out _config);
        }

        private void SaveConfig()
        {
            foreach(var component in _appComponents)
            {
                component.SaveToConfigObject(_config);
            }

            Serializer.ToXml("Config.xml", _config);
        }

        public static int CompareTimeStrings(string a, string b)
        {
            int a2 = -1;
            if(!GetTimeSeconds(a, out a2))
            {
                return a.CompareTo(b);
            }

            int b2 = -1;
            if(!GetTimeSeconds(b, out b2))
            {
                return a.CompareTo(b);
            }

            return a2.CompareTo(b2);
        }

        private static bool GetTimeSeconds(string text, out int time)
        {
            time = -1;

            var minutesSecondsRegEx = new Regex(@"(\d+):(\d+)");
            var match = minutesSecondsRegEx.Match(text);
            if(match.Success)
            {
                int m = int.Parse(match.Groups[1].Value);
                int s = int.Parse(match.Groups[2].Value);
                time = m * 60 + s;

                return true;
            }

            var secondsRegEx = new Regex(@"(\d+)");
            match = secondsRegEx.Match(text);
            if(secondsRegEx.IsMatch(text))
            {
                time = int.Parse(match.Groups[1].Value);

                return true;
            }

            return false;
        }

        private static bool GetOffsetSeconds(string text, out int time)
        {
            time = -1;

            var secondsRegEx = new Regex(@"(\d+)");
            var match = secondsRegEx.Match(text);
            if(secondsRegEx.IsMatch(text))
            {
                time = int.Parse(match.Groups[1].Value);

                return true;
            }

            return false;
        }

        private void OnRemoveDemoClicked()
        {
            if(_demoListView.SelectedItems.Count == 0)
            {
                LogWarning("You must select 1 or more demos from the list before you can remove them");
                return;
            }

            var demoIndices = new List<int>();
            foreach(var item in _demoListView.SelectedItems)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var displayInfo = listViewItem.Content as DemoDisplayInfo;
                if(displayInfo == null)
                {
                    continue;
                }

                var index = _demos.FindIndex(d => d == displayInfo.Demo);
                if(index >= 0 && index < _demos.Count)
                {
                    demoIndices.Add(index);
                }
            }

            demoIndices.Sort();
            demoIndices.Reverse();

            foreach(var demoIndex in demoIndices)
            {
                _demoListView.Items.RemoveAt(demoIndex);
                _demos.RemoveAt(demoIndex);
            }
        }

        public void OnCutByTimeContextClicked(ListView listView)
        {
            // @TODO:
        }

        public static void CopyListViewRowsToClipboard(ListView listView)
        {
            var stringBuilder = new StringBuilder();

            foreach(var item in listView.SelectedItems)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var realItem = listViewItem.Content;
                if(realItem == null)
                {
                    continue;
                }

                stringBuilder.AppendLine(realItem.ToString());
            }

            var allRows = stringBuilder.ToString();
            var allRowsFixed = allRows.TrimEnd(new char[] { '\r', '\n' });

            Clipboard.SetDataObject(allRowsFixed, true);
        }

        private void OnCancelJobClicked()
        {
            _parseArg.CancelOperation = 1;
            LogWarning("Job canceled!");
        }

        private void SetProgressThreadSafe(double value)
        {
            VoidDelegate valueSetter = delegate { _progressBar.Value = value; _progressBar.InvalidateVisual(); };
            _progressBar.Dispatcher.Invoke(valueSetter);
        }

        private static UDT_DLL.udtProtocol GetProtocolFromFilePath(string filePath)
        {
            var extension = Path.GetExtension(filePath).ToLower();
            if(!ProtocolFileExtDic.ContainsKey(extension))
            {
                return UDT_DLL.udtProtocol.Invalid;
            }

            var prot = ProtocolFileExtDic[extension];

            return ProtocolFileExtDic[extension];
        }

        private void LogMessage(string message, Color color)
        {
            VoidDelegate itemAdder = delegate 
            {
                var label = new Label();
                label.Foreground = new SolidColorBrush(color);
                label.Content = message;
                _logListBox.Items.Add(label);
                _logListBox.ScrollIntoView(label); 
            };

            _logListBox.Dispatcher.Invoke(itemAdder);
        }

        public void LogInfo(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(0, 0, 0));
        }

        public void LogWarning(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(255, 127, 0));
        }

        public void LogError(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(255, 0, 0));
        }

        private void DemoLoggingCallback(int logLevel, string message)
        {
            switch(logLevel)
            {
                case 3:
                    LogError("Critical error: " + message);
                    LogError("It is highly recommended you restart the application to not work with corrupt data in memory");
                    break;

                case 2:
                    LogError(message);
                    break;

                case 1:
                    LogWarning(message);
                    break;

                default:
                    LogInfo(message);
                    break;
            }
        }

        private void DemoProgressCallback(float progress, IntPtr userData)
        {
            SetProgressThreadSafe(100.0 * (double)progress);
        }

        private string GetLog()
        {
            var stringBuilder = new StringBuilder();

            foreach(var item in _logListBox.Items)
            {
                var label = item as Label;
                if(label == null)
                {
                    continue;
                }

                var line = label.Content as string;
                if(line == null)
                {
                    continue;
                }

                stringBuilder.AppendLine(line);
            }

            return stringBuilder.ToString();
        }

        private void ClearLog()
        {
            _logListBox.Items.Clear();
        }

        private void CopyLog()
        {
            Clipboard.SetDataObject(GetLog(), true);
        }

        private void CopyLogSelection()
        {
            var label = _logListBox.SelectedItem as Label;
            if(label == null)
            {
                return;
            }

            var line = label.Content as string;
            if(line == null)
            {
                return;
            }

            Clipboard.SetDataObject(line, true);
        }

        private void SaveLog()
        {
            using(var saveFileDialog = new System.Windows.Forms.SaveFileDialog())
            {
                saveFileDialog.InitialDirectory = System.Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                saveFileDialog.Filter = "text file (*.txt)|*.txt";
                if(saveFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                File.WriteAllText(saveFileDialog.FileName,  GetLog());
            }
        }

        private void FatalErrorHandler(string errorMessage)
        {
            throw new Exception(errorMessage);
        }
    }
}