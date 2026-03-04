from common.nn_model.eval_model import eval_model
from common.util.plot_curve import plot_curve
import datetime

def train_model_one_epoch(device, model, criterion, optimizer, loader):
    model.train()
    total_loss = 0.0
    correct = 0
    total = 0
    for img, labels in loader:
        img = img.to(device, non_blocking=True)
        labels = labels.to(device, non_blocking=True)
        outputs = model(img)
        loss = criterion(outputs, labels)
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        predict = outputs.argmax(dim=1)
        correct += (predict == labels).sum().item()
        total_loss += loss.item() * labels.size(0)
        total += labels.size(0)
    epoch_loss = total_loss / total
    epoch_acc = correct / total
    return epoch_loss, epoch_acc

def standard_epoch_log_fn(model, epoch, num_epochs, train_loss, train_acc, val_loss, val_acc):
    timestamp = datetime.datetime.now().strftime("%H:%M:%S")
    print(
        f"Timestamp: {timestamp} | "
        f"Epoch {epoch + 1}/{num_epochs} | "
        f"Train Acc: {train_acc:.4f}, Val Acc: {val_acc:.4f} | "
        f"Train Loss: {train_loss:.4f}, Val Loss: {val_loss:.4f}"
    )

def train_model(device, model, criterion, optimizer, scheduler, num_epochs, train_loader, val_loader, epoch_log_fn=standard_epoch_log_fn):
    print("Starting model training...")
    
    train_acc_values = []
    val_acc_values = []
    train_loss_values = []
    val_loss_values = []
    top_val_acc = 0.0
    top_epoch = 0
    top_model_state = None
    for epoch in range(num_epochs):
        train_loss, train_acc = train_model_one_epoch(device, model, criterion, optimizer, train_loader)
        train_loss_values.append(train_loss)
        train_acc_values.append(train_acc * 100)
        val_loss, val_acc = eval_model(device, model, criterion, val_loader)
        val_loss_values.append(val_loss)
        val_acc_values.append(val_acc * 100)
        epoch_log_fn(model, epoch, num_epochs, train_loss, train_acc, val_loss, val_acc)
        if val_acc > top_val_acc:
            top_val_acc = val_acc
            top_epoch = epoch
            top_model_state = model.state_dict().copy()
        if scheduler is not None:
            scheduler.step()

    if top_model_state is not None:
        model.load_state_dict(top_model_state)
        print(f"Top Val Acc: {top_val_acc:.4f}")

    print("Done")

    return {
        'train_acc_values': train_acc_values,
        'val_acc_values': val_acc_values,
        'train_loss_values': train_loss_values,
        'val_loss_values': val_loss_values,
        'top_val_acc': top_val_acc,
        'top_epoch': top_epoch
        }